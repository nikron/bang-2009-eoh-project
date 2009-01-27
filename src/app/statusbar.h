#ifndef __STATUSBAR_H
#define __STATUSBAR_H
#include<gtk/gtk.h>
/**
 * \param window The parent window of this statusbar.
 *
 * \brief Sets up a statusbar that displays bang events.
 *
 * \return The bang statusbar that got set up.
 */
GtkWidget* BMACHINE_setup_status_bar();
#endif
