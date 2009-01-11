/**
 * \file matrix-mult-module.c
 * \author Nikhil Bysani
 * \date January 11, 2008
 *
 * \brief Multiplies two matrices together.
 */
#include<stdlib.h>
#include<stdio.h>
#include"../base/bang-module-api.h"
#include<gtk/gtk.h>

static BANG_api api;

const char BANG_module_name[] = "matrix multiplication";
const unsigned char BANG_module_version[] = {0,0,1};

void GUI_init(GtkWidget **page, GtkWidget **page_label) {
	*page_label = gtk_label_new("Module of Matrix Multiplication");
	*page = gtk_vbox_new(FALSE,0);
	gtk_widget_show(*page_label);
	gtk_widget_show(*page);
}

int BANG_module_init(BANG_api get_api) {
	api = get_api;
	return 0;
}

void BANG_module_run() {
}
