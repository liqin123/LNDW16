#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "driver/uart.h"

static volatile os_timer_t channelHop_timer;

/*
 * Receives the characters from the serial port. Dummy to make the compiler happy.
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

void hop_channel(void *arg) {
    os_printf("old channel: %d\n", wifi_get_channel());
    int channel = wifi_get_channel() % 13 + 1;
    os_printf("new channel: %d\n", channel);
    wifi_set_channel(channel);
}

void 
promisc_cb(uint8 *buf, uint16 len)
{
    uint8 preamble[] = {len};
    uart1_tx_buffer(preamble, 1);
    uart1_tx_buffer(buf, len);
}

void ICACHE_FLASH_ATTR
sniffer_init_done() {
    os_printf("Enter: sniffer_init_done");
    wifi_station_set_auto_connect(false); // do not connect automatically
    wifi_station_disconnect(); // no idea if this is permanent
    wifi_promiscuous_enable(false);
    wifi_set_promiscuous_rx_cb(promisc_cb);
    wifi_promiscuous_enable(true);
    os_printf("done.\n");
    wifi_set_channel(1);
}

void ICACHE_FLASH_ATTR
user_init()
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    // lässt sämtliche os_printf calls ins leere laufen
    //system_set_os_print(0);
    wifi_set_opmode(0x1); // 0x1: station mode
    system_init_done_cb(sniffer_init_done);
    
    os_timer_disarm(&channelHop_timer);
    os_timer_setfn(&channelHop_timer, (os_timer_func_t *) hop_channel, NULL);
    os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL, 1);
}

