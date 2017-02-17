CFLAGS=-s -Wall -Wextra
CXXFLAGS=
LDFLAGS=-no-pie
LDLIBS=`pkg-config --cflags --libs appindicator3-0.1 gtk+-3.0 x11`

BIN=ambilight

all: clean module

.PHONY: clean module

clean:
	rm -f $(BIN)

module: ambilight_app.cpp ui.cpp util.cpp modes/ambilight.cpp modes/color.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -o $(BIN) $+ $(LDFLAGS) $(LDLIBS)