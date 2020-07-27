#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <X11/Xutil.h>

#include "ambilight.h"
#include "../constants.h"
#include "../ambilight_app.h"
#include "../modes.h"

Display *d = NULL;
XImage *current_image = NULL;
int screen_width;
int screen_height;

pthread_barrier_t bar_image_ready, bar_image_processed;

void start_ambilight_mode()
{
  // Prepare Xlib for multithreading
  XInitThreads();

  // Start ambilight updater thread
  pthread_t update_thread;
  pthread_create(&update_thread, NULL, &ambilight_updater, (void *)NULL);
}

void *ambilight_updater(__attribute__((unused)) void *arg)
{
  int worker_count = H_LEDS;

  fprintf(stderr, "Starting ambilight\n");

  // Read display attributes
  d = XOpenDisplay(NULL);
  XWindowAttributes gwa;
  XGetWindowAttributes(d, DefaultRootWindow(d), &gwa);
  screen_width = gwa.width;
  screen_width /= 2;
  screen_height = gwa.height;

  // Setup barriers
  pthread_barrier_init(&bar_image_ready, NULL, worker_count + 1);
  pthread_barrier_init(&bar_image_processed, NULL, worker_count + 1);

  // Create worker threads
  int workers_active = 1;
  pthread_t threads[worker_count];
  double region_width = (gwa.width / 2) / worker_count;
  for (int i = 0; i < worker_count; i++)
  {
    worker_args_t *args;
    args = (worker_args_t *)malloc(sizeof(worker_args_t));
    args->active = &workers_active;
    args->id = i;
    args->x = (int)(i * region_width);
    args->y = 48;
    args->width = (int)region_width;
    args->height = (int)(region_width * 2);
    pthread_create(&threads[i], NULL, ambilight_extraction_worker, (void *)args);
  }

  // Continuously refresh screen content
  while (1)
  {
    XImage *backup = current_image;
    current_image = XGetImage(d, DefaultRootWindow(d), 0, 0, screen_width, screen_height, AllPlanes, ZPixmap);

    pthread_barrier_wait(&bar_image_ready);

    if (backup)
    {
      XDestroyImage(backup);
    }

    if (get_mode() != MODE_AMBILIGHT)
    {
      // Stop worker threads
      workers_active = 0;
    }

    pthread_barrier_wait(&bar_image_processed);

    if (workers_active)
    {
      send_buffer_all();
      usleep(UPDATE_US);
    }
  }

  // Wait for worker threads to finish
  for (int i = 0; i < worker_count; i++)
  {
    pthread_join(threads[i], NULL);
  }

  pthread_barrier_destroy(&bar_image_ready);
  pthread_barrier_destroy(&bar_image_processed);

  XCloseDisplay(d);
  current_image = NULL;
  d = NULL;
  fprintf(stderr, "Exiting ambilight\n");
  return NULL;
}

void *ambilight_extraction_worker(void *data)
{
  worker_args_t *args = (worker_args_t *)data;
  int id = args->id;
  int global_x = args->x;
  int global_y = args->y;
  int width = args->width;
  int height = args->height;

  fprintf(stderr, "Worker %d starting\n", id);

  while (*args->active)
  {
    pthread_barrier_wait(&bar_image_ready);

    unsigned long red_mask = current_image->red_mask;
    unsigned long green_mask = current_image->green_mask;
    unsigned long blue_mask = current_image->blue_mask;

    unsigned long red_sum = 0;
    unsigned long green_sum = 0;
    unsigned long blue_sum = 0;

    for (int y = global_y; y < global_y + height; y++)
    {
      for (int x = global_x; x < global_x + width; x++)
      {
        unsigned long pixel = XGetPixel(current_image, x, y);

        unsigned char red = (pixel & red_mask) >> 16;
        unsigned char green = (pixel & green_mask) >> 8;
        unsigned char blue = pixel & blue_mask;

        red_sum += red;
        green_sum += green;
        blue_sum += blue;
      }
    }

    long num_of_pixels = width * height;

    unsigned int average_red = red_sum / num_of_pixels;
    unsigned int average_green = green_sum / num_of_pixels;
    unsigned int average_blue = blue_sum / num_of_pixels;

    // Update buffer with new value
    unsigned char *buffer = get_buffer();
    buffer[id * 3 + 0] = (int)(average_red * 0.8);
    buffer[id * 3 + 1] = (int)(average_green * 0.8);
    buffer[id * 3 + 2] = (int)(average_blue * 0.8);

    pthread_barrier_wait(&bar_image_processed);
  }
  fprintf(stderr, "Worker %d exiting\n", id);
  free(data);
  return NULL;
}