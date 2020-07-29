#pragma once

typedef struct worker_args
{
  int *active, id, x, y, width, height;
} worker_args_t;
void start_ambilight_mode();
void *ambilight_updater(void *arg);
void *ambilight_extraction_worker(void *data);
int index_max(unsigned int *array, int size);