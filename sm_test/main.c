#include <stdio.h>
#include <stdlib.h>
#include "uart_codec.h"

byte test_buffer[] = {1, 2, 3, START_BYTE_CASE, 1, START_BYTE_CASE, 2, 3, 4, 5, 6, 7, STOP_BYTE_CASE};
byte test_buffer_idx = 0;

byte internal_uart_buf_len() {
    return sizeof(test_buffer);
}

byte test_read(void) {
    return test_buffer[test_buffer_idx++];
}

byte test_flush(struct state *s) {
    printf("flush cb!\n");
    printf("Stop: I've read %d bytes of %d bytes.\n", s->already_read, s->next_packet_size);
    return s->already_read;
}

int main() {
    struct state *s = malloc(sizeof(struct state));
    init_state_machine(s);
    s->fifo_len = sizeof(test_buffer);
    s->read_cb = test_read;
    s->flush_cb = test_flush;
    while (s->fifo_len > 0) {
	s->next(s);
    }
    free(s);
}
