#include<gtk/gtk.h>
#include<stdlib.h>
#include"../base/bang.h"


static GtkWidget *statusbar = NULL;

static void bang_status(int signal, int num_args, void **args);

/**
 * \param signal The signal from the BANG library.
 * \param num_args The number of arguements of the signal
 * \param args Different depending on what signal this function caught.
 *
 * \brief Updates the statusbar depending on what signals it catches.
 */
static void bang_status(int signal, int num_args, void **args) {

	gdk_threads_enter();
	if (statusbar == NULL) {
		gdk_threads_leave();
		return;
	}

	guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),"bang_status");
	gtk_statusbar_pop(GTK_STATUSBAR(statusbar),context_id);

	if (signal == BANG_BIND_SUC) {

		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"!bang Machine has been bound.");
	} else if (signal == BANG_BIND_FAIL){

		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"!bang Machine could not bind.");
	} else if (signal == BANG_SERVER_STARTED) {

		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"!bang Machine server started.");
	} else if (signal == BANG_SERVER_STOPPED) {

		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"!bang Machine server stopped.");
	} else if (signal == BANG_PEER_ADDED) {

		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"A peer has been added.");
	} else if (signal == BANG_PEER_REMOVED) {

		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"A peer has been removed.");
	}

	/* It's recommended to flush before calling the leave function. */
	gdk_flush();
	gdk_threads_leave();

	int i = 0;
	for (i = 0; i < num_args; ++i) {
		free(args[i]);
	}

	free(args);
}

GtkWidget* BMACHINE_setup_status_bar() {
	statusbar = gtk_statusbar_new();

	BANG_install_sighandler(BANG_BIND_SUC,&bang_status);
	BANG_install_sighandler(BANG_BIND_FAIL,&bang_status);
	BANG_install_sighandler(BANG_SERVER_STARTED,&bang_status);
	BANG_install_sighandler(BANG_SERVER_STOPPED,&bang_status);
	BANG_install_sighandler(BANG_PEER_ADDED,&bang_status);
	BANG_install_sighandler(BANG_PEER_REMOVED,&bang_status);

	return statusbar;
}
