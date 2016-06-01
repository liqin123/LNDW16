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


struct state *global_uart_state;



LOCAL uint8 get_fifo_len() {
    return (READ_PERI_REG(UART_STATUS(UART0))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
}

LOCAL uint8 read_byte_cb() {
    return READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
}

LOCAL void ICACHE_FLASH_ATTR uart1_sendStr(const char *str)
{
    while(*str){
        uart_tx_one_char(UART1, *str++);
    }
}

byte my_flush_cb(struct state *s) {
    return 1;
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

void ICACHE_FLASH_ATTR
user_init()
{
    uart_init(BIT_RATE_3686400, BIT_RATE_115200);
    global_uart_state = (struct state *)os_malloc(sizeof(struct state));
    init_state_machine(global_uart_state);
    global_uart_state->read_cb = read_byte_cb;
    global_uart_state->flush_cb = my_flush_cb;
        
}

