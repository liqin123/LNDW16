CC=clang
all: main

main: main.o uart_codec.o
	$(CC)  main.o uart_codec.o -o $@

clean:
	rm -f *.o
