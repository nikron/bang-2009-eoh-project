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

void BMAHCHINE_change_server_status() {
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

	BANG_install_sighandler(BANG_LISTEN_FAIL,&server_status);
	BANG_install_sighandler(BANG_BIND_FAIL,&server_status);
	BANG_install_sighandler(BANG_BIND_SUC,&server_status);
	BANG_install_sighandler(BANG_GADDRINFO_FAIL,&server_status);
	BANG_install_sighandler(BANG_BIND_SUC,&server_status);
	BANG_install_sighandler(BANG_SERVER_STARTED,&server_status);
}
