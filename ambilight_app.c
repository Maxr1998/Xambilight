#include "ambilight_app.h"

#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "modes.h"
#include "ui.h"
#include "util.h"

#include "modes/ambilight.h"
#include "modes/color.h"

bool RUN_LOOP = true;

int current_mode = MODE_OFF;
bool handled = false;
int tty;

unsigned char *buffer = NULL;

int main()
{
  // Create UI
  pthread_t ui_thread;
  pthread_create(&ui_thread, NULL, &create_indicator, (void *)NULL);

  // Open connection
  char *path;
  char link_path[50];
  if (readlink(ARDUINO_PATH, link_path, 50) != -1)
  {
    path = (char *)malloc(100 + strlen(link_path));
    strcpy(path, "/dev/");
    strcpy(path + 5, link_path);
    printf("Resolved link, real path is %s\n", path);
  }
  else
  {
    path = (char *)malloc(strlen(ARDUINO_PATH) * sizeof(char) + 1);
    strcpy(path, ARDUINO_PATH);
    printf("Path is %s\n", path);
  }
  tty = open(path, O_RDWR | O_NOCTTY | O_SYNC);
  free(path);
  if (tty < 0)
  {
    pthread_cancel(ui_thread);
    printf("Communication error\n");
    return EXIT_FAILURE;
  }
  set_interface_attribs(tty, B115200, 0);
  set_blocking(tty, 1);

  buffer = (unsigned char *)malloc(BUFFER_SIZE);

  // Main loop
  while (RUN_LOOP)
  {
    if (!handled)
    {
      handled = true;
      usleep(MODE_PROPAGATION_DELAY); // Wait 200ms before state changes
      switch (current_mode)
      {
      case CMD_EXIT:
        RUN_LOOP = false;
        // fall through
      case MODE_OFF:
        send_mode_cmd(MODE_OFF);
        break;
      case MODE_AMBILIGHT:
        send_mode_cmd(MODE_OFF);
        send_mode_cmd(MODE_AMBILIGHT);
        start_ambilight_mode();
        break;
      case MODE_CYCLE:
        // ignore for now
        break;
      case MODE_FADE:
        send_mode_cmd(MODE_FADE);
        break;
      case MODE_COLOR:
        send_mode_cmd(MODE_COLOR);
        usleep(1000);
        send_color(254, 254, 104, 178);
        break;
      }
    }
    usleep(UPDATE_US);
  }
  free(buffer);
  close(tty);
  return EXIT_SUCCESS;
}

int get_mode()
{
  return current_mode;
}

void set_mode(int m)
{
  if (m == current_mode)
  {
    return;
  }
  current_mode = m;
  handled = false;
}

void send_mode_cmd(int mode)
{
  memset(buffer, 0, BUFFER_SIZE);
  buffer[0] = 77;
  buffer[1] = mode;
  buffer[2] = '\n';
  printf("Mode: %d\n", mode);
  send_buffer(3);
}

unsigned char *get_buffer()
{
  return buffer;
}

void send_buffer_all()
{
  send_buffer(BUFFER_SIZE);
}

void send_buffer(int c)
{
  write(tty, buffer, c);
  write(tty, "\n", 1);
}