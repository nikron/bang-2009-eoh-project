/**
 * \file main.c
 * \author Nikhil Bysani
 * \date Decemeber 20, 2009
 *
 * \brief The main function that sets up everything and contains most of the explanatory documentation.
 */

/**
 * \mainpage BANG Engineering Open House Project
 *
 * \section intro_sec Introduction
 *
 * This project is to build an application that can distribute an arbitrary  job/task/load
 * to other computers on the network.
 *
 * The structure of the program is supposed to be divided up into layers similarly to the
 * Model View Controller (MVC) model.  However, it wont really follow MVC that much.  Most
 * things are done through the project library, known as the BANG library.
 *
 * \section lib_sec BANG Library
 *
 * The library operates in the following manner:
 *
 * An application initializes it, and then installs signal handlers.  Those handlers will be able to
 * perform the vast majority of the actions that application would want to take based on the library.
 * However, there are a few exceptions, such as loading a module which critical for this project
 * to actually do something useful (loading a module will probably generate some signals!).
 */
#include"../base/core.h"
#include"../base/bang-signals.h"
#include"../base/bang-types.h"
#include<stdio.h>
#include<gtk/gtk.h>

void bind_suc(int signal, int sig_id, void *args) {
	fprintf(stderr,"The bind signal has been caught.\n");
}

void client_con(int signal, int sig_id, void *args) {
	fprintf(stderr,"A peer has connected.\n");
}

static gboolean delete_event(GtkWidget *widget, GdkEvent  *event, gpointer   data) {
	/* If you return FALSE in the "delete_event" signal handler,
	 * GTK will emit the "destroy" signal. Returning TRUE means
	 * you don't want the window to be destroyed.
	 * This is useful for popping up 'are you sure you want to quit?'
	 * type dialogs. */

	/* Change TRUE to FALSE and the main window will be destroyed with
	 * a "delete_event". */
	return TRUE;
}

static void destroy(GtkWidget *widget, gpointer data){
	///TODO: make bang_close actually close rather than wait forever.
	BANG_close();
	gtk_main_quit();
}

int main(int argc, char **argv) {
	GtkWidget *window;
	///TODO: Add this statusbar, at let it display the bind state.
	//GtkStatusbar *statusbar;

	gtk_init(&argc,&argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "!bang Machine");

	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(delete_event), NULL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);

	gtk_widget_show(window);

	BANG_init(&argc,argv);
	BANG_install_sighandler(BANG_BIND_SUC,&bind_suc);
	BANG_install_sighandler(BANG_PEER_CONNECTED,&client_con);

	gtk_main();
	return 0;
}
