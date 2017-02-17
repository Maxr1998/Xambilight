#include "color.hpp"

#include <algorithm>

#include "../ambilight_app.hpp"

void send_color(int brightness, int r, int g, int b) {
  std::fill(get_buffer(), get_buffer() + sizeof(get_buffer()), 0);
  get_buffer()[0] = 99;
  get_buffer()[1] = brightness;
  get_buffer()[2] = r;
  get_buffer()[3] = g;
  get_buffer()[4] = b;
  get_buffer()[5] = '\n';
  send_buffer(6);
}