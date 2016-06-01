CC=clang
all: simple esc_in_payload

simple: simple.o uart_codec.o
	$(CC)  simple.o uart_codec.o -o $@

esc_in_payload: esc_in_payload.o uart_codec.o
	$(CC)  esc_in_payload.o uart_codec.o -o $@

clean:
	rm -f *.o
