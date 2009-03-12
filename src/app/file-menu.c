#include"bang-machine-utils.h"
#include"../base/bang.h"
#include<gtk/gtk.h>
#include<stdlib.h>

void BMACHINE_open_module() {
	
	GtkWidget *dialog = gtk_file_chooser_dialog_new ("Open a Multiplicity module",NULL,GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
     
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		// do some stuff with filename
		g_free (filename);
       }
	
	gtk_widget_destroy (dialog);

	
}
/*
void BMACHINE_close_module(GtkWidget *widget) {
	
	
}*/

/* Shut down Multiplicity */
void BMACHINE_exit() {
	BANG_close();
	gtk_main_quit();
}
