CC=gcc
CFLAGS=-Wall -Wextra -std=gnu99
LDFLAGS=-no-pie
LDLIBS=`pkg-config --cflags --libs appindicator3-0.1 gtk+-3.0 x11`

BIN=ambilight

all: clean module

.PHONY: clean module

clean:
	rm -f $(BIN)

module: ambilight_app.c ui.c util.c modes/ambilight.c modes/color.c
	$(CC) $(CFLAGS) -o $(BIN) $+ $(LDFLAGS) $(LDLIBS)

module_test: ambilight_app.c ui.c util.c modes/ambilight.c modes/color.c
	$(CC) -DTEST_MODE $(CFLAGS) -o $(BIN) $+ $(LDFLAGS) $(LDLIBS)

test: module_test test.c
	$(CC) $(CFLAGS) -o test test.c $(LDFLAGS) `pkg-config --cflags --libs cairo x11 xrandr`