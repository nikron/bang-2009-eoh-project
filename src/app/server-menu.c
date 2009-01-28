#include"bang-machine-utils.h"
#include"../base/bang.h"
#include<gtk/gtk.h>
#include<stdlib.h>

/**
 * Start Stop Server
 */
static GtkWidget *ssserver = NULL;


static void server_status(int signal, int num_args, void **args) {

	gdk_threads_enter();
	if (ssserver == NULL) {
		gdk_threads_leave();
		return;
	}

	switch (signal) {
		case BANG_LISTEN_FAIL:
			BMACHINE_error_dialog("The server could not listen to its port.");
			break;
		case BANG_BIND_FAIL:
			BMACHINE_error_dialog("The server could not bind to its port.");
			break;
		case BANG_BIND_SUC:
			break;
		case BANG_GADDRINFO_FAIL:
			BMACHINE_error_dialog("The server could not fetch the address information.");
			break;
		case BANG_SERVER_STARTED:
			break;
	}

	gdk_threads_leave();


	int i = 0;
	for (i = 0; i < num_args; ++i) {
		free(args[i]);
	}

	free(args);
}

static void change_server_status(GtkWidget *widget) {
	if(BANG_is_server_running()) {
		BANG_server_stop();
		gtk_widget_set_sensitive(widget,FALSE);
	} else {
		BANG_server_start(NULL);
		gtk_widget_set_sensitive(widget,FALSE);
	}
}

GtkWidget* BMACHINE_setup_server_menu() {
	GtkWidget *server = gtk_menu_item_new_with_label("Server");
	GtkWidget *servermenu = gtk_menu_new();

	ssserver = gtk_image_menu_item_new_from_stock(GTK_STOCK_CONNECT,NULL);
	gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(ssserver))),"Start Server");
	g_signal_connect(G_OBJECT(ssserver), "activate", G_CALLBACK(change_server_status), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(servermenu),ssserver);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(server),servermenu);

	BANG_install_sighandler(BANG_LISTEN_FAIL,&server_status);
	BANG_install_sighandler(BANG_BIND_FAIL,&server_status);
	BANG_install_sighandler(BANG_BIND_SUC,&server_status);
	BANG_install_sighandler(BANG_GADDRINFO_FAIL,&server_status);
	BANG_install_sighandler(BANG_BIND_SUC,&server_status);
	BANG_install_sighandler(BANG_SERVER_STARTED,&server_status);

	return server;
}
