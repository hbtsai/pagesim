CC=gcc
CFLAGS=-c -Wall -g
LDFLAGS=
LFLAGS=-pthread -lm
SOURCES=pagesim.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=pagesim

all: $(SOURCES) $(EXECUTABLE) 

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LFLAGS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS) *.o *~
