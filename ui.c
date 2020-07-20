#include "ui.h"

#include <libappindicator/app-indicator.h>

#include "modes.h"

#define gtk_menu_append(menu, menu_item) gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item)

AppIndicator *indicator;

void *create_indicator(__attribute__((unused)) void *arg)
{
  gtk_init(0, NULL);
  indicator = app_indicator_new("ambilight-indicator", "", APP_INDICATOR_CATEGORY_HARDWARE);
  g_assert(IS_APP_INDICATOR(indicator));
  app_indicator_set_title(indicator, "Ambilight");
  //app_indicator_set_icon(indicator, "");
  app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ATTENTION);
  app_indicator_set_ordering_index(indicator, -2);
  app_indicator_set_menu(indicator, GTK_MENU(getMenu()));
  gtk_main();
  return NULL;
}

GtkWidget *getMenu()
{
  GtkWidget *menu_item = NULL;
  GtkWidget *menu = gtk_menu_new();

  menu_item = gtk_menu_item_new_with_label("Ambilight");
  g_signal_connect(menu_item, "activate", G_CALLBACK(change_mode), GINT_TO_POINTER(MODE_AMBILIGHT));
  gtk_menu_append(menu, menu_item);

  // Separator
  menu_item = gtk_separator_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), menu_item);

  menu_item = gtk_menu_item_new_with_label("Cycle");
  g_signal_connect(menu_item, "activate", G_CALLBACK(change_mode), GINT_TO_POINTER(MODE_CYCLE));
  gtk_menu_append(menu, menu_item);

  // Separator
  menu_item = gtk_separator_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), menu_item);

  menu_item = gtk_menu_item_new_with_label("Rainbow");
  g_signal_connect(menu_item, "activate", G_CALLBACK(change_mode), GINT_TO_POINTER(MODE_FADE));
  gtk_menu_append(menu, menu_item);

  // Separator
  menu_item = gtk_separator_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), menu_item);

  menu_item = gtk_menu_item_new_with_label("Fixed color");
  g_signal_connect(menu_item, "activate", G_CALLBACK(change_mode), GINT_TO_POINTER(MODE_COLOR));
  gtk_menu_append(menu, menu_item);

  menu_item = gtk_menu_item_new();
  GtkWidget *slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 255, 1);
  gtk_container_add(GTK_CONTAINER(menu_item), slider);
  gtk_widget_show_all(slider);
  gtk_menu_append(menu, menu_item);

  // Separator
  menu_item = gtk_separator_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), menu_item);

  menu_item = gtk_menu_item_new_with_label("Off");
  g_signal_connect(menu_item, "activate", G_CALLBACK(change_mode), GINT_TO_POINTER(MODE_OFF));
  gtk_menu_append(menu, menu_item);

  // Separator
  menu_item = gtk_separator_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), menu_item);

  menu_item = gtk_menu_item_new_with_label("Exit");
  g_signal_connect(menu_item, "activate", G_CALLBACK(exit_ui), NULL);
  gtk_menu_append(menu, menu_item);

  gtk_widget_show_all(menu);
  return menu;
}

void change_mode(__attribute__((unused)) GtkMenuItem *item, gpointer user_data)
{
  set_mode(GPOINTER_TO_INT(user_data));
}

void exit_ui(__attribute__((unused)) GtkMenuItem *item, __attribute__((unused)) gpointer user_data)
{
  g_object_unref(G_OBJECT(indicator)); // remove reference
  gtk_main_quit();
  set_mode(CMD_EXIT);
}