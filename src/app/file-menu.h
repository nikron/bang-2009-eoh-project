#ifndef __FILE_MENU_H
#define __FILE_MENU_H
#include<gtk/gtk.h>

typedef void (*GUI_module_init)(GtkWidget**,GtkWidget**);

void BMACHINE_open_module();

void BMACHINE_exit();

void BMACHINE_create_file_menu(GtkWidget *notebook);
#endif
