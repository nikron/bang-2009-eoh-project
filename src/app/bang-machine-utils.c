#include"bang-machine-utils.h"
#include<gtk/gtk.h>


void BMACHINE_error_dialog(char *error) {

	static GtkWidget *err_dialog = NULL;
	if (err_dialog) {
		gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(err_dialog),error);
	} else {
		err_dialog = gtk_message_dialog_new(NULL,
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				error);

	}

	gtk_dialog_run(GTK_DIALOG(err_dialog));
	gtk_widget_hide(err_dialog);
}
