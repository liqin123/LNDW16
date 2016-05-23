
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "user_config.h"
#include "uart.h"

#include "c_types.h"
#include "mem.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);
LOCAL os_timer_t network_timer;

void ICACHE_FLASH_ATTR wifi_config_ap();

int debug(char *text) {
}

//Main code function
static void ICACHE_FLASH_ATTR loop(os_event_t *events) {

  int c = uart0_rx_one_char();
  uart1_tx_one_char(c);
}


//Init function 
void ICACHE_FLASH_ATTR user_init() {

    // Set UART Speed (default appears to be rather odd 77KBPS)
    uart_init(BIT_RATE_115200,BIT_RATE_115200);

    gpio_init();
    // check GPIO setting (for config mode selection)
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);

    // enable UART on GPIO2
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
     
    //Start os task
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0 );

}
