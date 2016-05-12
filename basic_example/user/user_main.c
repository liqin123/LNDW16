#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

#define MAC_SIZE 6
uint8 original_mac_addr [MAC_SIZE] = {0, 0, 0, 0, 0, 0};
uint8 new_mac_addr[MAC_SIZE] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

const uint8 DST_IP[4] = {87, 106, 138, 10};
const uint8 DST_PORT = 3334;

struct espconn conn;
esp_udp udp;

// wifi: ESP has two "interfaces": one when acting as a station and another when it's acting as an AP

void ICACHE_FLASH_ATTR
wifi_callback( System_Event_t *evt ) {
    int i;
    memset(new_mac_addr, 0, MAC_SIZE);
    if (wifi_get_macaddr(STATION_IF, new_mac_addr) != true) {
        os_printf("Failed to get the new MAC address\n");
    }   
    os_printf("\n New MAC: ");
    for (i=0; i < MAC_SIZE; ++i) {
        os_printf("%x", new_mac_addr[i]);
    }   
    os_printf("Got an event!\n");
}

void send_datagram() {
    conn.type = ESPCONN_UDP;
    conn.state = ESPCONN_NONE;
    conn.proto.udp = &udp;
    IP4_ADDR((ip_addr_t *)conn.proto.udp->remote_ip, DST_IP[0], DST_IP[1], DST_IP[2], DST_IP[3]);
    conn.proto.udp->remote_port = DST_PORT;
    espconn_create(&conn);
    char *payload = "hallo";
    espconn_send(&conn, payload, strlen(payload));
    espconn_delete(&conn);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
    char ssid[32] = SSID;
    char password[64] = SSID_PASSWORD;
    struct station_config stationConf;

    //Set station mode
    wifi_set_opmode(STATION_MODE);

    // fix baud rate
    uart_div_modify( 0, UART_CLK_FREQ / ( 115200 ) );

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

    wifi_set_event_handler_cb(wifi_callback);
}
