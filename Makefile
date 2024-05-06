CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

all: server subscriber

common.o: common.c
	gcc $(CFLAGS) -c common.c

# Compileaza server.c
server: server.c common.o
	gcc $(CFLAGS) server.c common.o -o server

# Compileaza client.c
subscriber: subscriber.c common.o
	gcc $(CFLAGS) subscriber.c common.o -o subscriber

clean:
	rm -rf server subscriber *.o *.dSYM
