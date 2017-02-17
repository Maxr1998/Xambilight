#include "ambilight_app.hpp"

#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "modes.hpp"
#include "ui.hpp"
#include "util.hpp"

#include "modes/ambilight.hpp"
#include "modes/color.hpp"

#define ARDUINO_PATH "/dev/arduinoNano"
#define H_LEDS 30
#define BUFFER_SIZE H_LEDS * 3 * sizeof(char)

bool RUN_LOOP = true;

int current_mode = MODE_IDLE;
bool handled = false;
int tty;

unsigned char *buffer = NULL;

int main() {
  // Create UI
  pthread_t ui_thread;
  pthread_create(&ui_thread, NULL, create_indicator, (void*) NULL);

  // Open connection
  char *path;
  char link_path[50];
  if (readlink(ARDUINO_PATH, link_path, 50) != -1) {
    path = (char*) malloc(100 + strlen(link_path));
    strcpy(path, "/dev/");
    strcpy(path + 5, link_path);
    std::cout << "Resolved link, real path is " << path << "\n";
  } else {
    path = (char*) malloc(strlen(ARDUINO_PATH));
    strcpy(path, ARDUINO_PATH);
    std::cout << "Path is " << path << "\n";
  }
  tty = open(path, O_RDWR | O_NOCTTY | O_SYNC);
  free(path);
  if (tty < 0) {
    pthread_cancel(ui_thread);
    std::cout << "Communication error\n";
    return EXIT_FAILURE;
  }
  set_interface_attribs(tty, B115200, 0);
  set_blocking(tty, 1);

  buffer = (unsigned char*) malloc(BUFFER_SIZE);

  // Main loop
  while (RUN_LOOP) {
    if (!handled) {
      handled = true;
      usleep(DEFAULT_WAIT_MS * 2); // Wait 200ms before state changes
      switch (current_mode) {
        case MODE_IDLE: break; // do nothing
        case MODE_AMBILIGHT:
        send_mode_cmd(MODE_IDLE);
        send_mode_cmd(MODE_AMBILIGHT);
        start_ambilight_mode();
        break;
        case MODE_CYCLE:
        //send_buffer();
        break;
        case MODE_FADE:
        send_mode_cmd(MODE_FADE);
        break;
        case MODE_COLOR:
        send_mode_cmd(MODE_COLOR);
        usleep(1000);
        send_color(254, 254, 104, 178);
        break;
        case MODE_OFF:
        send_mode_cmd(MODE_IDLE);
        break;
        case MODE_EXIT:
        send_mode_cmd(MODE_IDLE);
        RUN_LOOP = false;
        break;
      }
    }
    usleep(UPDATE_MS);
  }
  free(buffer);
  close(tty);
  return EXIT_SUCCESS;
}

int get_mode() {
  return current_mode;
}

void set_mode(int m) {
  if (m == current_mode) {
    return;
  }
  current_mode = m;
  handled = false;
}

void send_mode_cmd(int mode) {
  std::fill(get_buffer(), get_buffer() + sizeof(get_buffer()), 0);
  buffer[0] = 77;
  buffer[1] = mode;
  buffer[2] = '\n';
  std::cout << "Mode: " << mode << "\n";
  send_buffer(3);
}

unsigned char *get_buffer() {
  return buffer;
}

void send_buffer() {
  send_buffer(BUFFER_SIZE);
}

void send_buffer(int c) {
  write(tty, buffer, c);
  write(tty, "\n", 1);
}