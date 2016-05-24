#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "driver/uart.h"


static volatile os_timer_t transmit_timer;
uint8 test_data[] = {1, 2, 3, 4, 5, 6};

/*
 * Receives the characters from the serial port.
 */

void ICACHE_FLASH_ATTR uart_rx_task(os_event_t *events) {
    if(events->sig == 0){ 
        uint8 fifo_len = (READ_PERI_REG(UART_STATUS(UART0))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
        uint8 d_tmp = 0;
        uint8 idx=0;
        for(idx=0;idx<fifo_len;idx++) {
            d_tmp = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
            // uart_tx_one_char(UART1, d_tmp);
        }
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);
        uart_rx_intr_enable(UART0);
    }
   

}

LOCAL void ICACHE_FLASH_ATTR uart1_sendStr(const char *str)
{
    while(*str){
        uart_tx_one_char(UART1, *str++);
    }
}

LOCAL void ICACHE_FLASH_ATTR transmit_cb(void *arg) {
    uint8 preamble[] = {6};
    uart1_tx_buffer(preamble, 1);
    uart1_tx_buffer(test_data, sizeof(test_data));
}

LOCAL void ICACHE_FLASH_ATTR set_transmit_timer(uint16_t interval) {
    // Start a timer for the flashing of the LED on GPIO 4, running continuously.
    os_timer_disarm(&transmit_timer);
    os_timer_setfn(&transmit_timer, (os_timer_func_t *)transmit_cb, (void *)0);
    os_timer_arm(&transmit_timer, interval, 1);
}


void ICACHE_FLASH_ATTR
user_init()
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    // lässt sämtliche os_printf calls ins leere laufen
    system_set_os_print(0);
    set_transmit_timer(1000);
    os_memset(test_data, 1, sizeof(test_data));
}

