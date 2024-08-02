CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -Os
LIBS = -lX11
BINDIR = /usr/local/bin/
TARGET = cstatusbar
OBJECTS = cstatusbar.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

gamepadcon.o: cstatusbar.c config.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJECTS)

install: $(TARGET)
	cp -f $(TARGET) $(BINDIR)
	chmod 755 $(BINDIR)/$(TARGET)

.PHONY: all clean install
