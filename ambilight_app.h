#pragma once

#include "modes.h"

#define ARDUINO_PATH "/dev/ttyACM0"
#define H_LEDS 30
#define BUFFER_SIZE H_LEDS * 3 * sizeof(char)

#define DEFAULT_WAIT_US 100000
#define UPDATE_US 30000

int get_mode();
void set_mode(int m);

void send_mode_cmd(int mode);
unsigned char *get_buffer();
void send_buffer_all();
void send_buffer(int c);