#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "driver/uart.h"

#define MAX_BUF_SIZE 200

uint8 next_packet_size = 0;
uint8 bytes_read = 0;
uint8 uart_buffer[MAX_BUF_SIZE];

/*
 * Receives the characters from the serial port.
 */
void uart_rx_task(os_event_t *events) {
    if(events->sig == 0){ 
        uint8 fifo_len = (READ_PERI_REG(UART_STATUS(UART0))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
	os_printf("Receiving %u bytes\n", fifo_len);
	uint8 d_tmp = 0;
	uint8 idx = 0;
        for(idx=0;idx<fifo_len;idx++) {
            d_tmp = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
	    os_printf("Got %u\n", d_tmp);
	    if (next_packet_size == 0) {
		os_printf("next_packet_size = %u\n", d_tmp);
		next_packet_size = d_tmp;
		idx++;
	    } else {
		uart_buffer[bytes_read] = d_tmp;
		os_printf("bytes_read=%u\n", bytes_read);
		os_printf("next_packet_size=%u\n", next_packet_size);
		bytes_read++; // sparen uns das -1, da wir vorher inkrementieren
		if (bytes_read == next_packet_size ) {
		    os_printf("I've read all %u bytes.\n", bytes_read);
		    bytes_read = 0;
		    next_packet_size = 0;
		}
	    }
	    //uart_tx_one_char(UART1, d_tmp);
        }
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);
        uart_rx_intr_enable(UART0);
    }
}

void ICACHE_FLASH_ATTR
user_init()
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
        
}

