RM = rm -rf
TARGETS = ll-cbuf.so
OBJECTS = cbuf.o cbuf-lua.o

all: $(TARGETS)

clean:
	$(RM) $(TARGETS) $(OBJECTS)

.PHONY: all clean

ll-cbuf.so: cbuf.o cbuf-lua.o
	gcc -O2 -shared -o $@ $^ -llua

%.o: %.c
	gcc -O2 -W -Wall -c -o $@ $<
