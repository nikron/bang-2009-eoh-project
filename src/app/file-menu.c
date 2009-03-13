#include"bang-machine-utils.h"
#include"file-menu.h"
#include"../base/bang.h"
#include<gtk/gtk.h>
#include<stdlib.h>

static GtkWidget *notebook = NULL;

static void deal_with_module(gchar *filename) {
	char *module_name;
	unsigned char *module_version;
	BANG_module* module;
	GtkWidget *page;
	GtkWidget *page_label;

	BANG_new_module(filename, &module_name, &module_version);
	module = BANG_get_module(module_name, module_version);

	GUI_module_init gui_module_init = BANG_get_symbol(module,"GUI_init");
	if (gui_module_init)
		gui_module_init(&page,&page_label);

	if (gui_module_init)
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook),page,page_label);

	BANG_run_module_in_registry(module_name, module_version);
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
