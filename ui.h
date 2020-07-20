#pragma once

#include <gtk/gtk.h>

void *create_indicator(void *arg);
GtkWidget *getMenu();
void change_mode(GtkMenuItem *item, gpointer user_data);
void exit_ui(GtkMenuItem *item, gpointer user_data);