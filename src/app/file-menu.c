#include"bang-machine-utils.h"
#include"../base/bang.h"
#include<gtk/gtk.h>
#include<stdlib.h>

static GtkWidget *notebook = NULL;

typedef void (*GUI_module_init)(GtkWidget**,GtkWidget**);

static void deal_with_module(gchar *filename) {
	char *module_name;
	unsigned char *module_version;
	BANG_module* module;

	BANG_new_module(filename, &module_name, &module_version);
	module = BANG_get_module(module_name, module_version);


	void* BANG_get_symbol(BANG_module *module, char *symbol);
}

void BMACHINE_open_module() {

	GtkWidget *dialog = gtk_file_chooser_dialog_new("Open a module",NULL,GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename;

		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
		deal_with_module(filename);
	}

	gtk_widget_destroy (dialog);
}

/* Shut down Multiplicity */
void BMACHINE_exit() {
	BANG_close();
	gtk_main_quit();
}

void BMACHINE_create_file_menu(GtkWidget *book) {
	notebook = book;
}
