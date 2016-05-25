#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "driver/uart.h"
#include "mem.h"


static volatile os_timer_t transmit_timer;
uint8 test_data[] = {1, 2, 3, 4, 5, 6};
uint8 test_data2[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
uint8 test_data3[120];
uint8 switch_flag=0;

#define START_BYTE_CASE 0x5f
#define STOP_BYTE_CASE 0xa0
#define ESCAPE_BYTE_CASE 0x55

uint8 start_byte[] = {START_BYTE_CASE};
uint8  stop_byte[] = {STOP_BYTE_CASE};
uint8  escape_byte[] = {ESCAPE_BYTE_CASE};



/*
 * Receives the characters from the serial port.
 */
LOCAL void ICACHE_FLASH_ATTR uart1_sendStr(const char *str)
{
    while(*str){
        uart_tx_one_char(UART1, *str++);
    }
}

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


void send_data(uint8 buffer[], uint8 len) {

    uint8 *tmp_buf = (uint8 *)os_malloc(len*2);
    uint16 new_len = 0;
    int i=0;
    for (i; i < len; i++, new_len++) {
	switch (buffer[i]) {
	case START_BYTE_CASE:
	        tmp_buf[new_len] = escape_byte[0];
		++new_len;
		break;
	case STOP_BYTE_CASE:
	    tmp_buf[new_len] = escape_byte[0];
		++new_len;
		break;
	case ESCAPE_BYTE_CASE:
	    	tmp_buf[new_len] = escape_byte[0];
		++new_len;
		break;
	default: break;
	}
	tmp_buf[new_len] =  buffer[i];
    }
    uart1_tx_buffer(start_byte, 1);
    uint8 length_byte[] = {new_len};
    uart1_tx_buffer(length_byte, 1);
    uart1_tx_buffer(tmp_buf, new_len);
    uart1_tx_buffer(stop_byte, 1);
    os_free(tmp_buf);
}

LOCAL void ICACHE_FLASH_ATTR transmit_cb(void *arg) {
    switch(switch_flag) {
    case 0:
	{
	    send_data(test_data, sizeof(test_data));
	    switch_flag = 1;
	    break;
	}
    case 1:
	{
	    send_data(test_data2, sizeof(test_data2));
	    switch_flag = 2;
	    break;
	}
    case 2:
	{
	    send_data(test_data3, sizeof(test_data3));
	    switch_flag = 0;
	    break;
	}
    }
    
}

LOCAL void ICACHE_FLASH_ATTR set_transmit_timer(uint16_t interval) {
    // Start a timer for the flashing of the LED on GPIO 4, running continuously.
    os_timer_disarm(&transmit_timer);
    os_timer_setfn(&transmit_timer, (os_timer_func_t *)transmit_cb, (void *)0);
    os_timer_arm(&transmit_timer, interval, 1);
    os_memset(test_data3, 2, sizeof(test_data3));
}


void ICACHE_FLASH_ATTR
user_init()
{
    uart_init(BIT_RATE_115200, BIT_RATE_3686400);
    // lässt sämtliche os_printf calls ins leere laufen
    system_set_os_print(0);
    set_transmit_timer(5);

}

