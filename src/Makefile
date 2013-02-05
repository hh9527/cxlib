RM = rm -rf
OBJECTS = cbuf.o

all: $(OBJECTS)

clean:
	$(RM) $(OBJECTS)

.PHONY: all clean

cbuf.o: cbuf.c
	gcc -W -Wall -c -o $@ $<
