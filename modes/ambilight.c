#define _GNU_SOURCE

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>

#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/XShm.h>

#include "ambilight.h"
#include "../constants.h"
#include "../ambilight_app.h"
#include "../modes.h"

XShmSegmentInfo shm_info;

XImage *current_image = NULL;

pthread_barrier_t bar_image_ready, bar_image_processed;

void start_ambilight_mode()
{
  // Prepare Xlib for multithreading
  XInitThreads();

  // Start ambilight updater thread
  pthread_t update_thread;
  pthread_create(&update_thread, NULL, &ambilight_updater, (void *)NULL);
  pthread_setname_np(update_thread, "updater");
}

void *ambilight_updater(__attribute__((unused)) void *arg)
{
  fprintf(stderr, "Starting ambilight\n");

  int worker_count = H_LEDS;

  // Setup X interfaces
  Display *d = XOpenDisplay(NULL);
  Screen *screen = DefaultScreenOfDisplay(d);
  Window root = screen->root;
  Visual *visual = screen->root_visual;
  int depth = screen->root_depth;

  // Query display dimensions
  XRRScreenResources *xrrsr = XRRGetScreenResources(d, root);
  RROutput primary = XRRGetOutputPrimary(d, root);
  XRROutputInfo *xrro = XRRGetOutputInfo(d, xrrsr, primary);
  if (xrro->connection != RR_Connected)
  {
    XRRFreeOutputInfo(xrro);
    XCloseDisplay(d);
    return NULL;
  }
  XRRCrtcInfo *xrrci = XRRGetCrtcInfo(d, xrrsr, xrro->crtc);

  int screen_width = xrrci->width, x = xrrci->x, y = xrrci->y;
  int region_width = screen_width / worker_count;
  int region_height = region_width * 4;

  XRRFreeCrtcInfo(xrrci);
  XRRFreeOutputInfo(xrro);
  XRRFreeScreenResources(xrrsr);

  // Check for XShmExtension
  int has_shm_extension = XShmQueryExtension(d);

  if (has_shm_extension)
  {
    fprintf(stderr, "Using X11 shared memory\n");
    current_image = XShmCreateImage(d, visual, depth, ZPixmap, NULL, &shm_info, screen_width, region_height);
    if (!current_image)
    {
      fprintf(stderr, "Can't create image!\n");
      return NULL;
    }
    shm_info.shmid = shmget(IPC_PRIVATE, current_image->bytes_per_line * current_image->height, IPC_CREAT | 0777);
    if (shm_info.shmid == -1)
    {
      fprintf(stderr, "Can't get shared memory!\n");
      return NULL;
    }
    shm_info.shmaddr = shmat(shm_info.shmid, NULL, SHM_RND);
    if (shm_info.shmaddr == (char *)-1)
    {
      fprintf(stderr, "Can't attach to shared memory!\n");
      return NULL;
    }
    current_image->data = shm_info.shmaddr;
    shm_info.readOnly = False;
    if (!XShmAttach(d, &shm_info))
    {
      fprintf(stderr, "Can't attach server to shared memory!\n");
      return NULL;
    }
  }

  // Setup barriers
  pthread_barrier_init(&bar_image_ready, NULL, worker_count + 1);
  pthread_barrier_init(&bar_image_processed, NULL, worker_count + 1);

  // Create worker threads
  int workers_active = True;
  pthread_t threads[worker_count];
  for (int i = 0; i < worker_count; i++)
  {
    worker_args_t *args;
    args = (worker_args_t *)malloc(sizeof(worker_args_t));
    args->active = &workers_active;
    args->id = i;
    args->x = x + i * region_width;
    args->y = y;
    args->width = region_width;
    args->height = region_height;
    pthread_create(&threads[i], NULL, ambilight_extraction_worker, (void *)args);
    char name[16];
    sprintf(name, "worker %d", i);
    pthread_setname_np(threads[i], name);
  }

  // Continuously refresh screen content
  XImage *backup = NULL;
  while (True)
  {
    if (has_shm_extension)
    {
      XShmGetImage(d, root, current_image, x, y + 60 /* Skip top panels and similar */, AllPlanes);
    }
    else
    {
      backup = current_image;
      current_image = XGetImage(d, root, x, y + 60 /* Skip top panels and similar */, screen_width, region_height, AllPlanes, ZPixmap);
    }

    pthread_barrier_wait(&bar_image_ready);

    if (backup)
    {
      XDestroyImage(backup);
    }

    if (get_mode() != MODE_AMBILIGHT)
    {
      // Stop worker threads
      workers_active = False;
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
  unsigned long num_of_pixels = width * height;
  unsigned long red_sum, green_sum, blue_sum;

  fprintf(stderr, "Worker %d starting\n", id);

  while (*args->active)
  {
    pthread_barrier_wait(&bar_image_ready);

    unsigned long red_mask = current_image->red_mask;
    unsigned long green_mask = current_image->green_mask;
    unsigned long blue_mask = current_image->blue_mask;

    // Reset sums
    red_sum = green_sum = blue_sum = 0;

    // Iterate through image
    for (int y = global_y; y < global_y + height; y++)
    {
      for (int x = global_x; x < global_x + width; x++)
      {
        unsigned long pixel = XGetPixel(current_image, x, y);

        red_sum += (pixel & red_mask) >> 16;
        green_sum += (pixel & green_mask) >> 8;
        blue_sum += pixel & blue_mask;
      }
    }

    // Update buffer with new value
    unsigned char *current_buffer = get_buffer() + id * 3;
    current_buffer[0] = (unsigned char)(red_sum / num_of_pixels);
    current_buffer[1] = (unsigned char)(green_sum / num_of_pixels);
    current_buffer[2] = (unsigned char)(blue_sum / num_of_pixels);

    pthread_barrier_wait(&bar_image_processed);
  }
  fprintf(stderr, "Worker %d exiting\n", id);
  free(data);
  return NULL;
}