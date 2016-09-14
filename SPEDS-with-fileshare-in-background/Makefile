CC=gcc
CFLAGS=-I.
SRCS =	serverSPED.c 
OBJS = $(SRCS:.cpp=.o)

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

s: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)