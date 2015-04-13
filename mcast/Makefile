.PHONY: mrouter recvmcast sendmcast clean

CFLAGS = -Wall -Werror

all: mrouter recvmcast sendmcast

mrouter:
	rm -f mrouter
	$(CC) $(CFLAGS) mrouter.c -o mrouter

recvmcast:
	rm -f recvmcast
	$(CC) $(CFLAGS) recvmcast.c -o recvmcast

sendmcast:
	rm -f sendmcast
	$(CC) $(CFLAGS) sendmcast.c -o sendmcast

clean:
	rm -f mrouter recvmcast sendmcast
