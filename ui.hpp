#ifndef __UI_H__
#define __UI_H__

#include <gtk/gtk.h>

void *create_indicator(void*);
GtkWidget* getMenu();
void change_mode(GtkMenuItem *item, gpointer user_data);
void exit_ui(GtkMenuItem *item, gpointer user_data);
#endif