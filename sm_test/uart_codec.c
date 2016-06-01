#include <stdio.h>
#include <stdlib.h>
#include "uart_codec.h"

state_fn start_fn, length_fn, length_esc_fn, read_data_fn, read_data_esc_fn;

byte start_byte = START_BYTE_CASE;
byte stop_byte = STOP_BYTE_CASE;
byte escape_byte = ESCAPE_BYTE_CASE;


byte read_next_byte(struct state *s) {
    s->fifo_len--;
    return s->read_cb();
}

void debug(const char *function_name, struct state *s) {
    printf("%s\n", function_name);
}


void reset_state(struct state *s) {
    debug(__func__, s);
    s->already_read = 0;
    s->next_packet_size = 0;
    // optional: memset(s->buffer, 0, BUFFER_SIZE)
}


void start_fn(struct state *s) {
    debug(__func__, s);
    byte current_byte = read_next_byte(s);
    switch (current_byte) {
    case START_BYTE_CASE:
	s->next = length_fn;
	break;
    default:
	break;
    }
}

void length_fn(struct state *s) {
    debug(__func__, s);
    byte current_byte = read_next_byte(s);
    switch (current_byte) {
    case START_BYTE_CASE:
    case STOP_BYTE_CASE:
	s->next = length_fn;
	break;
    case ESCAPE_BYTE_CASE:
	s->next = length_esc_fn;
	break;
    default:
	s->next_packet_size = current_byte;
	s->next = read_data_fn;
	break;
    }
}

void length_esc_fn(struct state *s) {
    debug(__func__, s);
    s->next_packet_size = read_next_byte(s);
    s->next = read_data_fn;
}

// Optimization: in der Funktion selbst loopen (spart function calls)
void read_data_fn(struct state *s) {
    debug(__func__, s);
    byte current_byte = read_next_byte(s);
    switch (current_byte) {
    case START_BYTE_CASE:
	reset_state(s);
	s->next = length_fn;
	break;
    case STOP_BYTE_CASE:
	s->flush_cb(s);
	reset_state(s);
	s->next = start_fn;
	break;
    case ESCAPE_BYTE_CASE:
	s->next = read_data_esc_fn;
	break;
    default:
	if (s->already_read > s->next_packet_size) {
	    printf("We're exceeding the new length\n"); // unescaped START in payload?
	}
	s->buffer[s->already_read] = current_byte;
	s->already_read++;
	break;
    }
}

void read_data_esc_fn(struct state *s) {
    debug(__func__, s);
    s->next = read_data_fn;
}

void init_state_machine(struct state *s) {
    s->next = start_fn;
}

/* Every occurrence of either the start, escape or stop sequence needs to be escaped by prepending an escape byte
   src: the data that needs to be escaped
   dst: the escaped data (2 * len)
   len: length of src
*/
byte escape_buffer(byte src[], byte dst[], byte len) {
    byte new_len = 0;
    int i;
    for (i=0;i < len; i++, new_len++) {
	switch (src[i]) {
	case START_BYTE_CASE:
	    dst[new_len] = escape_byte;
	    ++new_len;
	    break;
	case STOP_BYTE_CASE:
	    dst[new_len] = escape_byte;
	    ++new_len;
	    break;
	case ESCAPE_BYTE_CASE:
	    dst[new_len] = escape_byte;
	    ++new_len;
	    break;
	default: break;
	}
	dst[new_len] = src[i];
    }
    return new_len;
}

int send_packet(byte buffer[], byte len, void (*send_cb)(byte[], byte len)) {
    int byte_counter=0;
    byte *payload_buf = (byte *)malloc(len*2);
    byte actual_length_payload = escape_buffer(buffer, payload_buf, len);
    byte actual_length_buf[2];
    // we also need to escape the length byte
    byte actual_length_length = escape_buffer(&actual_length_payload, actual_length_buf, 1);
    send_cb(&start_byte, 1);
    send_cb(actual_length_buf, actual_length_length);
    send_cb(payload_buf, actual_length_payload);
    send_cb(&stop_byte, 1);
    free(payload_buf);
    byte_counter += (1 + 1 + actual_length_length + actual_length_payload);
    return byte_counter;
}
