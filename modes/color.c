#include <string.h>

#include "color.h"
#include "../constants.h"
#include "../ambilight_app.h"

void send_color(int brightness, int r, int g, int b)
{
  unsigned char *buffer = get_buffer();
  memset(buffer, 0, BUFFER_SIZE);
  buffer[0] = 99;
  buffer[1] = brightness;
  buffer[2] = r;
  buffer[3] = g;
  buffer[4] = b;
  buffer[5] = '\n';
  send_buffer(6);
}