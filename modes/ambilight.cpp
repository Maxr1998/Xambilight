#include "ambilight.hpp"

#include <iostream>
#include <pthread.h>
#include <unistd.h>

#include <X11/Xutil.h>

#include "../ambilight_app.hpp"
#include "../modes.hpp"

Display *d = NULL;
XImage *current_image = NULL;
int screen_width;
int screen_height;

void start_ambilight_mode() {
  // Prepare for multithreading, yay!
  XInitThreads();

  d = XOpenDisplay(NULL);
  XWindowAttributes gwa;
  XGetWindowAttributes(d, DefaultRootWindow(d), &gwa);
  screen_width = gwa.width;
  screen_width /= 2;
  screen_height = gwa.height;

  // Create update thread
  pthread_t update_thread;
  pthread_create(&update_thread, NULL, ambilight_updater, (void*) NULL);

  // Create worker threads
  pthread_t threads[30];
  double region_width = (gwa.width / 2) / 30;
  for (int i = 0; i < 30; i++) {
    struct ThreadParams *params;
    params = (struct ThreadParams*) malloc(sizeof(struct ThreadParams));
    params->id = i;
    params->x = (int) (i * region_width);
    params->y = 48;
    params->width = (int) region_width;
    params->height = (int) region_width;
    pthread_create(&threads[i], NULL, ambilight_extraction_worker, (void*) params);
  }
  std::cout << "Created ambilight threads\n";
}

void *ambilight_updater(void*) {
  std::cout << "Starting ambilight\n";
  while (get_mode() == MODE_AMBILIGHT) {
    XImage *backup = current_image;
    current_image = XGetImage(d, DefaultRootWindow(d), 0, 0, screen_width, screen_height, AllPlanes, ZPixmap);
    XFree(backup);

    usleep(UPDATE_MS);
    send_buffer();
  }
  usleep(DEFAULT_WAIT_MS); // Wait for all threads to exit
  XFree(current_image);
  XCloseDisplay(d);
  current_image = NULL;
  d = NULL;
  std::cout << "Exiting ambilight\n";
  return NULL;
}

void *ambilight_extraction_worker(void *data) {
  usleep(DEFAULT_WAIT_MS); // Wait for updater to have started

  struct ThreadParams *params = (struct ThreadParams*) data;
  int id = params->id;
  int global_x = params->x;
  int global_y = params->y;
  int width = params->width;
  int height = params->height;

  std::cout << "Worker " << id << " starting\n";

  while (get_mode() == MODE_AMBILIGHT) {
    unsigned long red_mask = current_image->red_mask;
    unsigned long green_mask = current_image->green_mask;
    unsigned long blue_mask = current_image->blue_mask;

    unsigned long red_sum = 0;
    unsigned long green_sum = 0;
    unsigned long blue_sum = 0;

    /*unsigned long value = 0;
    unsigned int vmin = 255;
    unsigned int vmax = 0;*/

    for (int y = global_y; y < global_y + height; y++) {
      for (int x = global_x; x < global_x + width; x++) {
        unsigned long pixel = XGetPixel(current_image, x, y);

        unsigned char red = (pixel & red_mask) >> 16;
        unsigned char green = (pixel & green_mask) >> 8;
        unsigned char blue = pixel & blue_mask;

        red_sum += red;
        green_sum += green;
        blue_sum += blue;

        /*int v = max(max(red, green), blue);
        vmin = min(vmin, v);
        vmax = max(vmax, v);
        value = value + v;*/
      }
    }

    long num_of_pixels = width * height;

    unsigned int average_red = red_sum / num_of_pixels;
    unsigned int average_green = green_sum / num_of_pixels;
    unsigned int average_blue = blue_sum / num_of_pixels;
    //unsigned int average_brightness = value / num_of_pixels;

    // Update buffer with new value
    get_buffer()[id * 3 + 0] = (int) (average_red * 0.8);
    get_buffer()[id * 3 + 1] = (int) (average_green * 0.8);
    get_buffer()[id * 3 + 2] = (int) (average_blue * 0.8);
    usleep(UPDATE_MS);
  }
  std::cout << "Worker " << id << " exiting\n";
  free(data);
  return NULL;
}