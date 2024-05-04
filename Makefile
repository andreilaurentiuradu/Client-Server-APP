CC = g++
CCFLAGS = -Wall -Wextra -std=c++17 -O0 -lm

build: server subscriber

run-server:
	./server

run-subscriber:
	./subscriber

server: server.cpp
	$(CPPC) -g server.cpp -o server $(CFLAGS)
	
subscriber: subscriber.cpp
	$(CPPC) -g subscriber.cpp -o subscriber $(CFLAGS)

