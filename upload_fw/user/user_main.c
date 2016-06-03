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

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

#define MAC_SIZE 6
uint8 original_mac_addr [MAC_SIZE] = {0, 0, 0, 0, 0, 0};

#ifndef CUSTOM_MAC
uint8 new_mac_addr[MAC_SIZE] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
#else // CUSTOM_MAC
uint8 new_mac_addr[MAC_SIZE] = {0x00, 0x11, 0x22, 0x33, 0x44, CUSTOM_MAC};
#endif // CUSTOM_MAC

uint8 ap_bssid[MAC_SIZE] = {0, 0, 0, 0, 0, 0};

const uint8 DST_IP[4] = {141, 76, 46, 34};
const uint16 DST_PORT = 3333;

struct espconn conn;
esp_udp udp;


LOCAL uint8 get_fifo_len() {
    return (READ_PERI_REG(UART_STATUS(UART0))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
}

LOCAL uint8 read_byte_cb() {
    return READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
}

/*
 * Receives the characters from the serial port.
 */
void uart_rx_task(os_event_t *events) {
    if(events->sig == 0) { 
	global_uart_state->fifo_len = get_fifo_len();
	while(global_uart_state->fifo_len > 0) {
	    global_uart_state->next(global_uart_state);
	}
	// reset UART interrupts       
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);
        uart_rx_intr_enable(UART0);
    }
}

byte send_datagram(struct uart_codec_state *s) {
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

    // potential overflow if we reconnect more than 10^10 times (but int size?)
    switch(espconn_send(&conn, s->buffer, s->already_read)) {
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
    espconn_delete(&conn);
}

void ICACHE_FLASH_ATTR
wifi_callback( System_Event_t *evt ) {
    os_printf("Got an event!\n");
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
                //send_datagram();
                break;
            }   
    }
}


//Init function 
void ICACHE_FLASH_ATTR
user_init()
{

    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    global_uart_state = (struct uart_codec_state*)os_malloc(sizeof(struct uart_codec_state));
    uart_codec_init(global_uart_state);
    global_uart_state->read_cb = read_byte_cb;
    global_uart_state->flush_cb = send_datagram;

    system_set_os_print(true);
    os_printf("ohai there\n");
    char ssid[32] = SSID;
    char password[64] = SSID_PASSWORD;
    struct station_config stationConf;

    //Set station mode
    wifi_set_opmode(STATION_MODE);

   
    if (wifi_get_macaddr(STATION_IF, original_mac_addr) != true) {
        os_printf("Failed to get the MAC address\n");
    } 

    if (wifi_set_macaddr(STATION_IF, new_mac_addr) != true) {
        os_printf("Failed to set the MAC address\n");
    }

    // connect to a Wifi (mobile hotspot in this case)
    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 64);
    wifi_station_set_config(&stationConf);
    wifi_station_connect();
    wifi_station_set_auto_connect(true);
    os_printf("last custom mac_byte=%x\n", new_mac_addr);

    wifi_set_event_handler_cb(wifi_callback);
    os_printf("bla blub\n");
}
