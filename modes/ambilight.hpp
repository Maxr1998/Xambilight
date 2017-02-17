#ifndef __AMBILIGHT_H__
#define __AMBILIGHT_H__

struct ThreadParams {
  int id, x, y, width, height;
};
void start_ambilight_mode();
void *ambilight_updater(void*);
void *ambilight_extraction_worker(void *data);
#endif