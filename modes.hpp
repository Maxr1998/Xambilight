#ifndef __MODES_H__
#define __MODES_H__

#define MODE_EXIT -1
#define MODE_IDLE 0
#define MODE_AMBILIGHT 1
#define MODE_CYCLE 2
#define MODE_FADE 3
#define MODE_COLOR 4
#define MODE_OFF 5

void set_mode(int m);
#endif