#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "driver/uart.h"

#define MAX_BUF_SIZE 500

uint16 next_packet_size = 0;
uint16 bytes_read = 0;
uint8 uart_buffer[MAX_BUF_SIZE];
int cnt=0;


uint8 sp_buf[200];

#define START_BYTE_CASE 0x5f
#define STOP_BYTE_CASE 0xa0
#define ESCAPE_BYTE_CASE 0x55

uint8 start_byte[] = {START_BYTE_CASE};
uint8  stop_byte[] = {STOP_BYTE_CASE};
uint8  escape_byte[] = {ESCAPE_BYTE_CASE};




LOCAL void ICACHE_FLASH_ATTR uart1_sendStr(const char *str)
{
    while(*str){
        uart_tx_one_char(UART1, *str++);
    }
}

/*
 * Receives the characters from the serial port.
 */
void uart_rx_task(os_event_t *events) {
    if(events->sig == 0){ 
        uint8 fifo_len = (READ_PERI_REG(UART_STATUS(UART0))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
	//os_printf("Receiving %u bytes\n", fifo_len);
	uint8 d_tmp = 0;
	uint8 idx = 0;
	
        for(idx=0;idx<fifo_len;idx++) {
            d_tmp = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
	    //os_printf("Got %u\n", d_tmp);
	    // 6 & 9 are from the tx test code
	    
	    if (next_packet_size != 0 && next_packet_size != 6 && next_packet_size != 9 && next_packet_size != 120) {
		os_sprintf(sp_buf, "Error in next line!\n");
		uart1_sendStr(sp_buf);
	    }
	    if (next_packet_size == 0) {
		next_packet_size = d_tmp;
		
		os_sprintf(sp_buf, "next_packet_size=%u\n", next_packet_size);
		uart1_sendStr(sp_buf);
	      	// os_printf("next_packet_size=%u\n", next_packet_size);
		os_sprintf(sp_buf, "counter=%d\n", cnt);
		uart1_sendStr(sp_buf);
		cnt++;
	    } else {
		uart_buffer[bytes_read] = d_tmp;
		bytes_read++; // sparen uns das -1, da wir vorher inkrementieren
		if (bytes_read == next_packet_size ) {
		    //os_printf("I've read all %u bytes.\n", bytes_read);
		    #if 0
		    int i=0;
		    for (i; i < bytes_read; i++) {
			os_printf("buffer[%u]=%u", i, uart_buffer[i]);
		    }
		    #endif 
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
    uart_init(BIT_RATE_3686400, BIT_RATE_115200);
        
}

