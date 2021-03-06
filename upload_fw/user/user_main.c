#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "driver/uart.h"
#include "driver/uart_codec.h"
#include "mem.h"

struct uart_codec_state *global_uart_state;
static int ssl_connected = 0;
static os_timer_t sslTimer;
static os_timer_t auth_timer;
static int uart_counter = 0;
static int got_ip = 0;

void send_datagram (uint8 *msg, uint8 len);

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

#define MAC_SIZE 6
uint8 original_mac_addr [MAC_SIZE] = {0, 0, 0, 0, 0, 0};

#ifndef DEVID
#define DEVID 0x1
#endif // DEVID

static int datagram_count = 0;
uint8 new_mac_addr[MAC_SIZE] = {0x00, 0x21, 0x2e, 0x00, 0x00, DEVID};
uint8 ap_bssid[MAC_SIZE] = {0, 0, 0, 0, 0, 0};

const uint8 DST_IP[4] = {141, 76, 46, 34};
const uint16 DST_PORT = 3333;

const char *VPNWEB_IP = "141.30.1.225";
const uint16 VPNWEB_PORT = 443;

struct espconn conn, tcpConn;
esp_udp udp;
esp_tcp tcp;

#define POST_REQ_SIZE 130
const char *post_req_prefix = "POST / HTTP/1.1\r\nHost: 141.30.1.225\r\nAccept: */*\r\nContent-Length: 55\r\n\r\nusername=ESP";
const char *post_req_suffix = "%40gast&password=Nalee1ye&buttonClicked=4";
char *post_req;


LOCAL uint8 get_fifo_len() {
    return (READ_PERI_REG(UART_STATUS(UART0))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
}

LOCAL uint8 read_byte_cb() {
    return READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
}

#define CTL_START 0x5f
#define CTL_STOP  0xa0
#define CTL_ESC   0x55

// FIXME: Localize
#define BUFLEN 1000
typedef enum { st_start, st_len, st_read, st_esc } state_t;
typedef enum { r_done, r_running } result_t;

typedef struct
{
    state_t state;
    uint8 length;
    uint8 pos;
    uint8 buffer[BUFLEN];
} uartrx_fsm_t;

static uartrx_fsm_t uart1rx_fsm = { .state = st_start, .length = 0, .pos = 0 };

uint8 rx_step (uartrx_fsm_t *fsm, uint8 b)
{
    uint8 result = r_running;

    //os_printf ("rx_step: state=%d, byte=%2x\n", fsm->state, b);

    switch (fsm->state)
    {
        case st_start:
            fsm->pos = 0;
            fsm->length = 0;
            if (b == CTL_START)
            {
                fsm->state = st_len;
            } else
            {
                // Stay in start
            }
            break;

        case st_len:
            fsm->length = b;
            fsm->state = st_read;
            break;

        case st_read:
            if (b == CTL_ESC)
            {
                fsm->state = st_esc;
            } else if (b == CTL_START)
            {
                fsm->state = st_len;
            } else if (b == CTL_STOP)
            {
                fsm->state = st_start;
                result = r_done;
            } else
            {
                if (fsm->pos < BUFLEN)
                {
                    fsm->buffer[fsm->pos++] = b;
                } else
                {
                    fsm->state = st_start;
                    os_printf ("Buffer overflow\n");
                }
            }
            break;

        case st_esc:
            fsm->state = st_read;
            fsm->buffer[fsm->pos++] = b;
            break;
    }
    return result;
}

void reset_uart_rx_intr (void)
{
    WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);
    uart_rx_intr_enable(UART0);
}

void uart_rx_task(os_event_t *events)
{
    int i, fifo_len;
    uint8 data_byte, result;

    // os_printf ("Event %d\n", events->sig);

    if (events->sig == 0)
    { 
        fifo_len = get_fifo_len();
        for (i = 0; i < fifo_len; i++)
        {
            data_byte = read_byte_cb();
            result = rx_step (&uart1rx_fsm, data_byte);
            if (result == r_done)
            {
                os_printf ("Done reading UART [%d] len field=%d, bytes read=%d\n", ++uart_counter, uart1rx_fsm.length, uart1rx_fsm.pos);
                send_datagram (uart1rx_fsm.buffer, uart1rx_fsm.pos);
            }
        }
        reset_uart_rx_intr ();
    }
}

void connectCB(void *arg) {
    int rv;

    os_timer_disarm (&sslTimer);
    os_printf("Disarmed SSL timer\n");

    os_printf("we have connected to VPNWEB\n");
    os_printf("sending POST request:\n==================\n%s\n==================\n", post_req);
    rv = espconn_secure_send(&tcpConn, (uint8*)post_req, strlen(post_req));
    os_printf("POST request done with rv=%d\n", rv);
    if (rv == 0)
    {
        ssl_connected = 1;
    }

    espconn_delete (&tcpConn);
    os_timer_arm (&auth_timer, 900000, 0);
}

void errorCB(void *arg, sint8 err) {
    os_printf("We have an error: %d\n", err);
}

void authenticate_wifi (void *arg)
{
    int result;

    result = espconn_secure_set_size (0x01 /* client */, 6000);
    if (!result)
    {
        os_printf ("Adjusting SSL buffer size failed\n\r");
        return;
    }

    tcpConn.type = ESPCONN_TCP;
    tcpConn.state = ESPCONN_NONE;
    tcpConn.proto.tcp = &tcp;
    tcpConn.proto.tcp->remote_port = VPNWEB_PORT;
    //*((uint32 *)tcpConn.proto.tcp->remote_ip) = ipaddr_addr(VPNWEB_IP);
    tcpConn.proto.tcp->remote_ip[0] = 141;
    tcpConn.proto.tcp->remote_ip[1] = 30;
    tcpConn.proto.tcp->remote_ip[2] = 1;
    tcpConn.proto.tcp->remote_ip[3] = 225;
    espconn_regist_connectcb(&tcpConn, connectCB);
    espconn_regist_reconcb(&tcpConn, errorCB);

    // Arm SSL timer
    os_printf("Arming SSL timer (%dms)\n", SSL_TIMEOUT_MS);
    os_timer_arm (&sslTimer, SSL_TIMEOUT_MS, 0);
    ssl_connected = 0;
    espconn_secure_connect(&tcpConn);
    os_printf("We have asked for a connection.\n");
}

void send_datagram (uint8 *msg, uint8 len) {

    if (!ssl_connected)
    {
        os_printf ("Ignore datagram - no SSL\n");
        return;
    }

    conn.type = ESPCONN_UDP;
    conn.state = ESPCONN_NONE;
    conn.proto.udp = &udp;
    IP4_ADDR((ip_addr_t *)conn.proto.udp->remote_ip, DST_IP[0], DST_IP[1], DST_IP[2], DST_IP[3]);
    conn.proto.udp->remote_port = DST_PORT;
    switch(espconn_create(&conn)) {
        case ESPCONN_ARG:
            {
                os_printf("_create: invalid argument\n");
                break;
            }
        case ESPCONN_ISCONN:
            {
                os_printf("_create: already connected\n");
                break;
            }
        case ESPCONN_MEM:
            {
                os_printf("_create: out of memory\n");
                break;
            }
    }

    uint8 *buffer = (uint8*)os_malloc(len + 1);
    buffer[0] = DEVID;
    memcpy (buffer + 1, msg, len);
    
    switch(espconn_send(&conn, buffer, len + 1)) {
        case ESPCONN_ARG:
            {
                os_printf("_send: invalid argument\n");
                break;
            }
        case ESPCONN_MEM:
            {
                os_printf("_send: OoM\n");
                break;
            }
    }

    os_printf ("Sent datagram #%d\n", ++datagram_count);
    espconn_delete (&conn);
    os_free (buffer); 
}

void //ICACHE_FLASH_ATTR
wifi_callback( System_Event_t *evt ) {
    //os_printf("Got an event!\n");
    switch (evt->event) {
        case EVENT_STAMODE_CONNECTED:
            {
	      os_printf("connect to ssid %s, channel %d\n",
                        evt->event_info.connected.ssid,
                        evt->event_info.connected.channel);
	      memcpy(ap_bssid, evt->event_info.connected.bssid, 6);
	      break;
            }
        case EVENT_STAMODE_DISCONNECTED:
            {   
                os_printf("disconnect from ssid %s, reason %d\n",
                        evt->event_info.disconnected.ssid,
                        evt->event_info.disconnected.reason);
                break;
		
            }   

        case EVENT_STAMODE_GOT_IP:
            {   
                os_printf("We have an IP\n");
                os_timer_arm (&auth_timer, 1000, 0);
                got_ip = 1;
                break;
            }   
    }
}

void sslTimerCallback (void *pArg)
{
    system_restart();
}

void initDoneCb(void) {
char ssid[32] = SSID;
    char password[64] = SSID_PASSWORD;
    struct station_config stationConf;

    // insert DEVID into POST request
    post_req = (char *)os_malloc(POST_REQ_SIZE);

    // FIXME: Seems like %2.2u crashes so we need to produce trailing 0 by hand
    if (DEVID > 9)
    {
        os_sprintf(post_req, "%s%u%s", post_req_prefix, DEVID, post_req_suffix);
    } else
    {
        os_sprintf(post_req, "%s0%u%s", post_req_prefix, DEVID, post_req_suffix);
    }

    //Set station mode
    wifi_set_opmode(STATION_MODE);


    // Set SSL timer callback
    os_timer_setfn (&sslTimer, sslTimerCallback, NULL);
   
    if (wifi_get_macaddr(STATION_IF, original_mac_addr) != true) {
        os_printf("Failed to get the MAC address\n");
    } 

    // generate and set custom MAC
    // 00:21:2e:<rand>:<rand>:DEVID
    new_mac_addr[3] = os_random() % 256;
    new_mac_addr[4] = os_random() % 256;
    
    if (wifi_set_macaddr(STATION_IF, new_mac_addr) != true) {
        os_printf("Failed to set the MAC address\n");
    }

    // connect to a Wifi
    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 64);
    wifi_station_set_config(&stationConf);
    wifi_station_connect();

    wifi_station_set_auto_connect(true);
    os_printf("last custom mac_byte=%x\n", new_mac_addr);

    os_timer_disarm (&auth_timer);
    os_timer_setfn (&auth_timer, (os_timer_func_t *) authenticate_wifi, NULL);

    wifi_set_event_handler_cb(wifi_callback);
         
    //wifi_set_sleep_type(LIGHT_SLEEP_T);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    system_set_os_print(true);
    system_init_done_cb(initDoneCb);
}
