#ifndef __AMBILIGHT_APP_H__
#define __AMBILIGHT_APP_H__

#include "modes.hpp"

#define DEFAULT_WAIT_MS 100000
#define UPDATE_MS 30000

int get_mode();
void set_mode(int m);

void send_mode_cmd(int mode);
unsigned char *get_buffer();
void send_buffer();
void send_buffer(int c);
#endif