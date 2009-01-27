#include<gtk/gtk.h>
#include"../base/bang.h"

static GtkWidget *server = NULL;
static GtkWidget *servermenu = NULL;
/**
 * Start Stop Server
 */
static GtkWidget *ssserver = NULL;
static GtkWidget *server_pref = NULL;

static void change_server_status() {
	if(BANG_is_server_running()) {
		BANG_server_stop();
		/* gtk_widget_set_sensitive(widget,FALSE); */
	} else {
		BANG_server_start(NULL);
		/* gtk_widget_set_sensitive(widget,FALSE); */
	}
}


GtkWidget* BMACHINE_setup_server_menu() {
	server = gtk_menu_item_new_with_label("Server");
	servermenu = gtk_menu_new();

	ssserver = gtk_image_menu_item_new_from_stock(GTK_STOCK_CONNECT,NULL);
	gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(ssserver))),"Start Server");
	g_signal_connect(G_OBJECT(ssserver), "activate", G_CALLBACK(change_server_status), NULL);

	server_pref = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES,NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(servermenu),ssserver);
	gtk_menu_shell_append(GTK_MENU_SHELL(servermenu),server_pref);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(server),servermenu);

	return server;
}
