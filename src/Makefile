RM = rm -rf
TARGETS = cbuf.so
OBJECTS = cbuf.o cbuf-lua.o

all: $(TARGETS)

clean:
	$(RM) $(TARGETS) $(OBJECTS)

.PHONY: all clean

cbuf.so: cbuf.o cbuf-lua.o
	gcc -shared -o $@ $^ -llua

%.o: %.c
	gcc -W -Wall -c -o $@ $<
