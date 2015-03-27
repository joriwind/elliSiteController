CC=gcc
CFLAGS=-Wall -DWOLFSSL_DTLS
LIBS=-lwolfssl -lpthread

all: dtls-interface


server-dtls: dtls-interface.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o client-udp client-dtls client-dtls-nonblocking client-dtls-resume server-udp server-dtls server-dtls-nonblocking server-dtls-threaded
