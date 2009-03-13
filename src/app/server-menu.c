#include"bang-machine-utils.h"
#include"../base/bang.h"
#include<gtk/gtk.h>
#include<stdlib.h>

/**
 * Start Stop Server
 */
static GtkWidget *ssserver = NULL;

static void net_status(int signal, int num_args, void **args) {

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
			BMACHINE_error_dialog("Could not fetch the address information.");
			break;
		case BANG_SERVER_STARTED:
			break;
		case BANG_CONNECT_FAIL:
			BMACHINE_error_dialog("Could not make a socket for connection.");
	}

	gdk_threads_leave();


	int i = 0;
	for (i = 0; i < num_args; ++i) {
		free(args[i]);
	}

	free(args);
}

static void connect_to_peer(GtkDialog *dialog, gint response_id) {
	if (response_id == 1) {
		GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
		GList *node = gtk_container_get_children(GTK_CONTAINER(content_area));
		GtkWidget *entry = node->data;

		const gchar *host = gtk_entry_get_text(GTK_ENTRY(entry));
		BANG_connect_thread(host);

	}

	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void BMACHINE_connect_peer() {
	GtkWidget *input_dialog = gtk_dialog_new();
	GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(input_dialog));
	GtkWidget *entry = gtk_entry_new_with_max_length(300);
	GtkWidget *ok = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget *cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

	gtk_box_pack_start(GTK_BOX(content_area), entry, FALSE, FALSE, 0);
	gtk_dialog_add_action_widget(GTK_DIALOG(input_dialog), ok, 1);
	gtk_dialog_add_action_widget(GTK_DIALOG(input_dialog), cancel, 2);

	gtk_widget_show_all(input_dialog);
	g_signal_connect(G_OBJECT(input_dialog),"response",G_CALLBACK(connect_to_peer),NULL);

	gtk_dialog_run(GTK_DIALOG(input_dialog));
}

void BMACHINE_change_server_status() {
	if (ssserver) {
		if(BANG_is_server_running()) {
			BANG_server_stop();
			gtk_widget_set_sensitive(ssserver,FALSE);
		} else {
			BANG_server_start(NULL);
			gtk_widget_set_sensitive(ssserver,FALSE);
		}
	}
}

void BMACHINE_setup_server_menu(GtkWidget *widget) {
	ssserver = widget;

	BANG_install_sighandler(BANG_LISTEN_FAIL,&net_status);
	BANG_install_sighandler(BANG_BIND_FAIL,&net_status);
	BANG_install_sighandler(BANG_BIND_SUC,&net_status);
	BANG_install_sighandler(BANG_GADDRINFO_FAIL,&net_status);
	BANG_install_sighandler(BANG_BIND_SUC,&net_status);
	BANG_install_sighandler(BANG_SERVER_STARTED,&net_status);
	BANG_install_sighandler(BANG_CONNECT_FAIL,&net_status);
}
