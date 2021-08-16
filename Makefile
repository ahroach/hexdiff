CC = gcc
CFLAGS = -O3

MAIN = hexdiff
SRCS = hexdiff.c
OBJS = $(SRCS:.c=.o)

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o $(MAIN)
