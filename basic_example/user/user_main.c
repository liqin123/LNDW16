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

// wifi: ESP has two "interfaces": one when acting as a station and another when it's acting as an AP


//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
    // clearing new_mac_addr so we'll get fresh values
    memset(new_mac_addr, 0, MAC_SIZE);
    int i;
    os_printf("\n Original MAC: ");
    for (i=0; i < MAC_SIZE; ++i) {
        os_printf("%x", original_mac_addr[i]);
    }
    if (wifi_get_macaddr(STATION_IF, new_mac_addr) != true) {
        os_printf("Failed to get the new MAC address\n");
    } 
    os_printf("\n New MAC: ");
    for (i=0; i < MAC_SIZE; ++i) {
        os_printf("%x", new_mac_addr[i]);
    }
    os_delay_us(2 * 1000 * 1000);
    system_os_post(user_procTaskPrio, 0, 0 );
}

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

    //Start os task
    /*
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0 );
    */
}
