CC=gcc
CFLAGS=-Wall

SRCDIR=src
OBJDIR=obj
SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

BINDIR=bin
BIN=$(BINDIR)/regen

LIBDIR=lib
LIB=$(LIBDIR)/libregen.so

all: $(BIN)

debug: CFLAGS=-Wall -g -rdynamic -O0 -DDEBUG -fsanitize=address
debug: clean
debug: $(BIN)

release: CFLAGS=-Wall -O2 -DNDEBUG
release: clean
release: $(BIN)

lib: CFLAGS=-Wall -O2 -DNDEBUG -fpic -shared
lib: $(eval OBJS = $(filter-out $(OBJDIR)/main.o, $(OBJS)))
lib: clean
lib: $(LIB)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -lm -o $@

$(LIB): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -lm -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -r $(OBJDIR)/* $(BIN) $(LIB)