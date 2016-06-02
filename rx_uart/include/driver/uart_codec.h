#ifndef _UART_CODEC_H
#define _UART_CODEC_H

typedef unsigned char byte;

#define START_BYTE_CASE 0x5f
#define STOP_BYTE_CASE 0xa0
#define ESCAPE_BYTE_CASE 0x55

#define BUFFER_SIZE 250

byte TEST_SEQUENCE[15]; // adapt length length (stupid compiler)
byte TEST_SEQUENCE_LEN;
struct uart_codec_state;
typedef void state_fn(struct uart_codec_state *);

void uart_codec_init(struct uart_codec_state *);

struct uart_codec_state {
    state_fn *next;
    byte next_packet_size;
    byte buffer[BUFFER_SIZE];
    // muss immer aktuell sein, beim Interrupt entsprechend gesetzt werden
    byte fifo_len;
    byte already_read;
    byte (*read_cb)(void); // callback by user, provides next byte
    byte (*flush_cb)(struct uart_codec_state *s); // callback invoked when we read a valid STOP byte
    
};

int uart_codec_send_packet(const byte buffer[], byte len, void (*send_cb)(byte[], byte len));


#endif // _UART_CODEC_H
