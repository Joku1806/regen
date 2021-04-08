CC=gcc
CFLAGS=-g -Wall
OBJS=VLA.o parser.o generator.o main.o
BIN=regen

all: $(BIN)

regen: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -lm -o regen

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -r regen *.o