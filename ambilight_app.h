#pragma once

#include "modes.h"

#define ARDUINO_PATH "/dev/ttyACM0"

#define MODE_PROPAGATION_DELAY 200000

int get_mode();
void set_mode(int m);

void send_mode_cmd(int mode);
unsigned char *get_buffer();
void send_buffer_all();
void send_buffer(int c);