/**
 * \file main.c
 * \author nikhil Bysani
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
 *
 * \section GTK
 *
 * The first(last?) GUI we make will be in gtk.  However, it turns out that gtk sucks.  However, it is the main
 * graphical took kit on my system, so we use it.  Anyway, gtk programs are susposed to be single threaded.  Obviously
 * this a recipe for failure.  So, we invoke some not often used methods to acquire the global gdk lock.  But since
 * it is not often used, we may encounter problems.  Well, cross your fingers and hope for the best.
 *
 * Turns out the main failure of this is that it will be even harder to port this program to windows because gtk is
 * _not_ at all thread safe in windows.
 *
 * Basic look of the application. menubar, tabbed views, statusbar.  Tabs included, but not limited to, listing of a
 * the peers and their status.  And then a tab for each module.
 */
#include"../base/bang.h"
#include"menus.h"
#include"preferences.h"
#include"statusbar.h"
#include"server-menu.h"
#include"tabs.h"
#include<locale.h>
#include<stdio.h>
#include<stdlib.h>
#include<glib.h>
#include<gtk/gtk.h>
#include<assert.h>

typedef void (*GUI_module_init)(GtkWidget**,GtkWidget**);

static GtkWidget *window;
static GtkWidget *vbox;
static GtkWidget *notebook;
static GtkWidget *peers_page_label;

/**
 * GTK callback when the window is about to be deleted, does not to be locked as that comes
 * automagically with all GTK functions.
 */
static gboolean delete_event() {
	/* If you return FALSE in the "delete_event" signal handler,
	 * GTK will emit the "destroy" signal. Returning TRUE means
	 * you don't want the window to be destroyed.
	 * This is useful for popping up 'are you sure you want to quit?'
	 * type dialogs. */

	/* Change TRUE to FALSE and the main window will be destroyed with
	 * a "delete_event". */
	BANG_close();
	BMACHINE_close_status_bar();
	return FALSE;
}

/**
 * GTK callback when the window is destroyed, does not to be locked as that comes
 * automagically with all GTK functions.
 */
static void destroy() {
	gdk_flush();
	gtk_main_quit();
}

int main(int argc, char **argv) {
	/* Set up our library. */
	setlocale(LC_ALL,"");
	BANG_init(&argc,argv);

	/* Note:  gtk expects that as a process, you do not need to free its memory
	 So, it lets the operating system free all memory when the process closes. */
	g_thread_init(NULL);
	gdk_threads_init();
	gtk_init(&argc,&argv);

	/* Set up the window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "!bang Machine");
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(delete_event), NULL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);

	vbox = gtk_vbox_new(FALSE,0);

	/* Set up the netebook */
	notebook = gtk_notebook_new();

	peers_page_label = gtk_label_new("Peers");
	/* gtk_notebook_append_page(GTK_NOTEBOOK(notebook),peers_page_label,peers_page_label); */


	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook),TRUE);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook),TRUE);

	GtkWidget *statusbar = BMACHINE_setup_status_bar();

	/*
	 * set up the layout of the top level window
	 */
	BMACHINE_create_menus(vbox,window);
	gtk_box_pack_end(GTK_BOX(vbox),statusbar,FALSE,FALSE,0);

	/*
	 * make the box the container for the window
	 */
	gtk_container_add(GTK_CONTAINER(window),vbox);

	/*
	 * Max the window for now rather than reading in last used
	 */
	//gtk_window_maximize(GTK_WINDOW(window));


	gtk_widget_show_all(window);

	gtk_main();
	return 0;
}
