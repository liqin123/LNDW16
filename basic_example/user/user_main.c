#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

#define MAC_SIZE 6
uint8 mac_addr [MAC_SIZE] = {0, 0, 0, 0, 0, 0};

// wifi: ESP has two "interfaces": one when acting as a station and another when it's acting as an AP


//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
    int i;
    os_printf("\n MAC: ");
    for (i=0; i < MAC_SIZE; ++i) {
        os_printf("%x", mac_addr[i]);
    }
    os_delay_us(2 * 1000 * 1000);
    system_os_post(user_procTaskPrio, 0, 0 );
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


    uart_div_modify( 0, UART_CLK_FREQ / ( 115200 ) );
    //Start os task
    if (wifi_get_macaddr(STATION_IF, mac_addr) != true) {
        os_printf("Failed to get the MAC address\n");
    } 
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

    system_os_post(user_procTaskPrio, 0, 0 );
}
