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
#include<stdlib.h>
#include<glib.h>
#include<gtk/gtk.h>

GtkWidget *window;
GtkWidget *vbox;
GtkWidget *statusbar;
GtkWidget *menubar;
GtkWidget *file;
GtkWidget *filemenu;
GtkWidget *open_module;
GtkWidget *server;
GtkWidget *servermenu;
///Start Stop Server
GtkWidget *ssserver;

/*
 * \param signal The signal from the BANG library.
 * \param sig_id Useless... mah library already has deprecated features!
 * \param args Different depending on what signal this function caught.
 *
 * \brief Updates the statusbar depending on what signals it catches.
 * NOTE: GTK IS NOT THREAD SAFE!  Umm, let me think how we should fix this.
 */
void bind_status(int signal, int sig_id, void *args) {
	gdk_threads_enter();
	guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),"bind_status");
	gtk_statusbar_pop(GTK_STATUSBAR(statusbar),context_id);
	if (signal == BANG_BIND_SUC) {
		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"!bang Machine has been bound.");
	} else {
		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"!bang Machine could not bind.");
	}
	gdk_threads_leave();
	free(args);
}

void client_con(int signal, int sig_id, void *args) {
	gdk_threads_enter();
	guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),"peer_status");
	gtk_statusbar_pop(GTK_STATUSBAR(statusbar),context_id);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"A peer has connected.");
	gdk_threads_leave();
	free(args);
}

static void change_server_status(GtkWidget *widget, gpointer data) {
	if(BANG_is_server_running()) {
		BANG_server_stop();
		gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(widget))),"Start Server");
	} else {
		BANG_server_start(NULL);
		gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(widget))),"Stop Server");
	}
}


static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
	/* If you return FALSE in the "delete_event" signal handler,
	 * GTK will emit the "destroy" signal. Returning TRUE means
	 * you don't want the window to be destroyed.
	 * This is useful for popping up 'are you sure you want to quit?'
	 * type dialogs. */

	/* Change TRUE to FALSE and the main window will be destroyed with
	 * a "delete_event". */
	return FALSE;
}

static void destroy(GtkWidget *widget, gpointer data) {
	BANG_close();
	gtk_main_quit();
}

int main(int argc, char **argv) {

	BANG_init(&argc,argv);
	BANG_install_sighandler(BANG_BIND_SUC,&bind_status);
	BANG_install_sighandler(BANG_BIND_FAIL,&bind_status);
	BANG_install_sighandler(BANG_PEER_CONNECTED,&client_con);

	///Note:  gtk expects that as a process, you do not need to free its memory
	///So, it lets the operating system free all memory when the process closes.
	g_thread_init(NULL);
	gdk_threads_init();
	gtk_init(&argc,&argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "!bang Machine");
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(delete_event), NULL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);

	vbox = gtk_vbox_new(FALSE,0);

	statusbar = gtk_statusbar_new();

	menubar = gtk_menu_bar_new();

	file = gtk_menu_item_new_with_label("File");
	filemenu = gtk_menu_new();
	open_module = gtk_menu_item_new_with_label("Open Module");

	gtk_menu_shell_append(GTK_MENU_SHELL(filemenu),open_module);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file),filemenu);

	server = gtk_menu_item_new_with_label("Server");
	servermenu = gtk_menu_new();
	ssserver = gtk_menu_item_new_with_label("Start Server");
	g_signal_connect(G_OBJECT(ssserver), "activate", G_CALLBACK(change_server_status), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(servermenu),ssserver);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(server),servermenu);

	gtk_menu_append(menubar,file);
	gtk_menu_append(menubar,server);

	gtk_box_pack_start(GTK_BOX(vbox),menubar,FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(vbox),statusbar,FALSE,FALSE,0);

	gtk_container_add(GTK_CONTAINER(window),vbox);

	gtk_window_maximize(GTK_WINDOW(window));

	gtk_widget_show(ssserver);
	gtk_widget_show(servermenu);
	gtk_widget_show(server);
	gtk_widget_show(open_module);
	gtk_widget_show(filemenu);
	gtk_widget_show(file);
	gtk_widget_show(menubar);
	gtk_widget_show(statusbar);
	gtk_widget_show(vbox);
	gtk_widget_show(window);

	gtk_main();
	return 0;
}
