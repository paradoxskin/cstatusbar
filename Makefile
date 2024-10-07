CC = cc
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -std=c99 -pedantic -Wall -Os ${CPPFLAGS}
LIBS = -lX11
BINDIR = /usr/local/bin/
TARGET = cstatusbar
OBJECTS = cstatusbar.o

all: $(TARGET)

debug: CFLAGS+=-DDEBUG=1 -g
debug: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

gamepadcon.o: cstatusbar.c config.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJECTS)

install: $(TARGET)
	cp -f $(TARGET) $(BINDIR)
	chmod 755 $(BINDIR)/$(TARGET)

.PHONY: all clean install debug
