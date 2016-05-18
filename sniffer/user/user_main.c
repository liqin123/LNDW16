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
#define printmac(buf, i) os_printf("\t%02X:%02X:%02X:%02X:%02X:%02X", buf[i+0], buf[i+1], buf[i+2], \
				    buf[i+3], buf[i+4], buf[i+5])
static volatile os_timer_t channelHop_timer;

static void loop(os_event_t *events);
static void promisc_cb(uint8 *buf, uint16 len);
                                          


static void ICACHE_FLASH_ATTR
promisc_cb(uint8 *buf, uint16 len)
{
    os_printf("promisc_cb: channel=%3d, len=%d", wifi_get_channel(), len);
    printmac(buf,  4);
    printmac(buf, 10);
    printmac(buf, 16);
    os_printf("\n");
}

//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
    os_delay_us(10);
}

void ICACHE_FLASH_ATTR
sniffer_init_done() {
    wifi_station_set_auto_connect(false); // do not connect automatically
    wifi_station_disconnect(); // no idea if this is permanent
    wifi_promiscuous_enable(false);
    wifi_set_promiscuous_rx_cb(promisc_cb);
    wifi_promiscuous_enable(true);
    os_printf("done.\n");

    wifi_set_channel(1);
}
//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
  uart_div_modify( 0, UART_CLK_FREQ / ( 115200 ) );
    os_delay_us(100);

    os_printf(" -> Promisc mode setup ... ");
    
    wifi_set_opmode(0x1); // 0x1: station mode
    os_printf(" -> Init finished!\n\n");
    system_init_done_cb(sniffer_init_done);
}

