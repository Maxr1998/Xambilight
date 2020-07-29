CC=gcc
APP_DEPS=appindicator3-0.1 gtk+-3.0 x11 xrandr
TEST_DEPS=cairo x11 xrandr
CFLAGS=-Wall -Wextra -std=gnu99 `pkg-config --cflags $(DEPS)`
LDFLAGS=-no-pie
LDLIBS=`pkg-config --libs $(DEPS)`

OBJS=ambilight_app.o ui.o util.o modes/ambilight.o modes/color.o
APP=ambilight
TEST_TOOL=test

all: $(APP)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^

# Set correct deps for targets
$(APP): DEPS = $(APP_DEPS)
$(TEST_TOOL): DEPS = $(TEST_DEPS)

$(APP): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS) -lpthread

$(TEST_TOOL): test.o $(APP)
	$(CC) -o $@ $< $(LDFLAGS) $(LDLIBS)

.PHONY: clean

clean:
	find . -name "*.o" -delete
	rm -f $(APP) $(TEST_TOOL)