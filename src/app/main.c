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
#include"../base/core.h"
#include"../base/bang-signals.h"
#include"../base/bang-types.h"
#include"../base/bang-module.h"
#include"../base/bang-module-api.h"
#include"server-preferences.h"
#include<stdio.h>
#include<stdlib.h>
#include<glib.h>
#include<gtk/gtk.h>

GtkWidget *window;
GtkWidget *vbox;
GtkWidget *notebook;
GtkWidget *peers_page_label;
GtkWidget *statusbar;
GtkWidget *menubar;
GtkWidget *file;
GtkWidget *filemenu;
GtkWidget *open_module;
GtkWidget *server;
GtkWidget *servermenu;
///Start Stop Server
GtkWidget *ssserver;
GtkWidget *server_pref;

/*
 * \param signal The signal from the BANG library.
 * \param sig_id Useless... mah library already has deprecated features!
 * \param args Different depending on what signal this function caught.
 *
 * \brief Updates the statusbar depending on what signals it catches.
 * NOTE: GTK IS NOT THREAD SAFE!  Umm, let me think how we should fix this.
 */
//bang callback, gtk needs to be locked
void server_status(int signal, int sig_id, void *args) {
	gdk_threads_enter();
	guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),"server_status");
	gtk_statusbar_pop(GTK_STATUSBAR(statusbar),context_id);
	if (signal == BANG_BIND_SUC) {
		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"!bang Machine has been bound.");
	} else if (signal == BANG_BIND_FAIL){
		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"!bang Machine could not bind.");
		gtk_widget_set_sensitive(ssserver,TRUE);
		gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(ssserver))),"Start Server");
	} else if (signal == BANG_SERVER_STARTED) {
		gtk_widget_set_sensitive(ssserver,TRUE);
		gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(ssserver))),"Stop Server");
		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"!bang Machine server started.");
	} else {
		gtk_widget_set_sensitive(ssserver,TRUE);
		gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(ssserver))),"Start Server");
		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"!bang Machine server stopped.");
	}
	///It's recommended to flush before calling the leave function.
	gdk_flush();
	gdk_threads_leave();
	free(args);
}

//bang callback, gtk needs to be locked
void client_con(int signal, int sig_id, void *args) {
	///We could make this buffer smaller.
	char buf[30];
	gdk_threads_enter();
	guint context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),"peer_status");
	gtk_statusbar_pop(GTK_STATUSBAR(statusbar),context_id);
	if (signal == BANG_PEER_CONNECTED) {
		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,"A peer has connected.");
	} else {
		sprintf(buf,"Peer %d has been removed.",*((int*)args));
		gtk_statusbar_push(GTK_STATUSBAR(statusbar),context_id,buf);
	}
	gdk_flush();
	gdk_threads_leave();
	free(args);
}

static void open_bang_module(GtkWidget *widget, gpointer data) {
	///OMG THE LINE IS SO LONG,OH SHI--
	GtkWidget *get_module = gtk_file_chooser_dialog_new("Open Module", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	if (gtk_dialog_run(GTK_DIALOG(get_module)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(get_module));
		BANG_module *module = BANG_load_module(filename);
		BANG_run_module(module);
		BANG_unload_module(module);
		free(filename);
	}
	gtk_widget_destroy(get_module);
}

//gtk callback, does not need to get locked is automatic
static void change_server_status(GtkWidget *widget, gpointer data) {
	///Just make the stop button inactive.  We'll make it active when we get the signal.
	if(BANG_is_server_running()) {
		BANG_server_stop();
		gtk_widget_set_sensitive(widget,FALSE);
	} else {
		BANG_server_start(NULL);
		gtk_widget_set_sensitive(widget,FALSE);
	}
}

//gtk callback, does not need to get locked is automatic
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

//gtk callback, does not need to get locked is automatic
static void destroy(GtkWidget *widget, gpointer data) {
	gdk_flush();
	BANG_close();
	gtk_main_quit();
}

int main(int argc, char **argv) {

	//Set up our library.
	BANG_init(&argc,argv);
	BANG_install_sighandler(BANG_BIND_SUC,&server_status);
	BANG_install_sighandler(BANG_BIND_FAIL,&server_status);
	BANG_install_sighandler(BANG_SERVER_STARTED,&server_status);
	BANG_install_sighandler(BANG_SERVER_STOPPED,&server_status);
	BANG_install_sighandler(BANG_PEER_CONNECTED,&client_con);
	BANG_install_sighandler(BANG_PEER_REMOVED,&client_con);

	///Note:  gtk expects that as a process, you do not need to free its memory
	///So, it lets the operating system free all memory when the process closes.
	g_thread_init(NULL);
	gdk_threads_init();
	gtk_init(&argc,&argv);

	///Set up the windwo
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "!bang Machine");
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(delete_event), NULL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);

	vbox = gtk_vbox_new(FALSE,0);

	///Set up the netebook
	notebook = gtk_notebook_new();

	peers_page_label = gtk_label_new("Peers");
	///gtk_notebook_append_page(GTK_NOTEBOOK(notebook),peers_page_label,peers_page_label);

	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook),TRUE);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook),TRUE);

	///Set up the statusbar
	statusbar = gtk_statusbar_new();

	///Set up the menubar
	menubar = gtk_menu_bar_new();

	file = gtk_menu_item_new_with_label("File");
	filemenu = gtk_menu_new();
	open_module = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN,NULL);
	gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(open_module))),"Open Module");
	g_signal_connect(G_OBJECT(open_module),"activate",G_CALLBACK(open_bang_module),NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(filemenu),open_module);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file),filemenu);

	server = gtk_menu_item_new_with_label("Server");
	servermenu = gtk_menu_new();
	ssserver = gtk_image_menu_item_new_from_stock(GTK_STOCK_CONNECT,NULL);
	gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(ssserver))),"Start Server");
	g_signal_connect(G_OBJECT(ssserver), "activate", G_CALLBACK(change_server_status), NULL);

	server_pref = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES,NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(servermenu),ssserver);
	gtk_menu_shell_append(GTK_MENU_SHELL(servermenu),server_pref);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(server),servermenu);

	gtk_menu_append(menubar,file);
	gtk_menu_append(menubar,server);

	///set up the layout of the top level window
	gtk_box_pack_start(GTK_BOX(vbox),menubar,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox),notebook,FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(vbox),statusbar,FALSE,FALSE,0);

	///make the box the container for the window
	gtk_container_add(GTK_CONTAINER(window),vbox);

	///Max the window for now rather than reading in last used
	gtk_window_maximize(GTK_WINDOW(window));

	///Show the menubar and its contents.
	gtk_widget_show(server_pref);
	gtk_widget_show(ssserver);
	gtk_widget_show(servermenu);
	gtk_widget_show(server);
	gtk_widget_show(open_module);
	gtk_widget_show(filemenu);
	gtk_widget_show(file);
	gtk_widget_show(menubar);

	///Show the statusbar
	gtk_widget_show(statusbar);

	//Show the notebook and its contents
	gtk_widget_show(peers_page_label);
	gtk_widget_show(notebook);

	//Show the window
	gtk_widget_show(vbox);
	gtk_widget_show(window);

	gtk_main();
	return 0;
}
