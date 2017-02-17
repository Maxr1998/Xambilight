#ifndef __UTIL_H__
#define __UTIL_H__

int min(int a, int b);
int max(int a, int b);
int set_interface_attribs(int fd, int speed, int parity);
void set_blocking(int fd, int should_block);
#endif