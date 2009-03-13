#ifndef __SERVER_MENU_H
#define __SERVER_MENU_H
#include<glib.h>
#include<gtk/gtk.h>
GtkWidget *notebook;
GtkWidget *peerlist;

/**
 * \brief Adds a signal handler for PEER_ADDED events - calls update peer tree.
 */

void BMACHINE_tabs_init();

/**
 * \brief Closes and frees the net part of the library.
 * 
 * \arg Path to module.
 */

void BMACHINE_open_module_tab(char *filename);

/**
 * \brief Updates peer tree.
 */

void update_peer_tree();
#endif