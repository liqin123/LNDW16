#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uart_codec.h"

// possible output of our encode function
byte test_buffer[] = {START_BYTE_CASE, 15, 2, 3, 1, ESCAPE_BYTE_CASE, START_BYTE_CASE, 2, 3, 4, 5, 6, 7};
byte test_buffer2[] = {2, 3, 5, 1, STOP_BYTE_CASE};

// merged the two buffers, without sequence bytes ( used for sanity check at the end)
byte test_total[] = {2, 3, 1, START_BYTE_CASE, 2, 3, 4, 5, 6, 7, 2, 3, 5, 1};

byte test_buffer_idx = 0;
byte test_buffer_idx2 = 0;
struct state *s;

byte test_read(void) {
    printf("Reading: %x\n", test_buffer[test_buffer_idx]);
    return test_buffer[test_buffer_idx++];
}

byte test_read2(void) {
    printf("Reading: %x\n", test_buffer2[test_buffer_idx2]);
    return test_buffer2[test_buffer_idx2++];
}

byte test_flush(struct state *s) {
    printf("flush cb!\n");
    printf("Stop: I've read %d bytes of %d bytes.\n", s->already_read, s->next_packet_size);
    printf("Comparing src with dst: %d\n", memcmp(test_total, s->buffer, sizeof(test_total)));
    int i=0;
    for (; i < sizeof(test_total); i++) {
	printf("test_total[i]=%x vs s->buffer[i]=%x\n", test_total[i], s->buffer[i]);
    }
    return s->already_read;
}

int main() {
    printf("split buffers \n");
    
    // reconstructing the data
    struct state *s = malloc(sizeof(struct state));
    init_state_machine(s);
    s->fifo_len = sizeof(test_buffer);
    s->read_cb = test_read;
    s->flush_cb = test_flush;
    while (s->fifo_len > 0) {
	s->next(s);
    }
    printf("read first buffer. Switching callback\n");
    // initial uart fifo is empty, state is saved, another interrupt fired, SM continues
    s->read_cb = test_read2;
    s->fifo_len = sizeof(test_buffer2);
    while (s->fifo_len > 0) {
	s->next(s);
    }
    free(s);
    
}
