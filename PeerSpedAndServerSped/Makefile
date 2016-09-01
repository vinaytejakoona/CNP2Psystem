CC=gcc
CFLAGS=-I.
SRCS =	peerSPED.c 
OBJS = $(SRCS:.cpp=.o)

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

p: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)