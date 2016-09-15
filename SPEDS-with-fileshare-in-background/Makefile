CC=g++
CFLAGS=-I.
SRCS =	peer.c 
OBJS = $(SRCS:.cpp=.o)

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

p: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)
