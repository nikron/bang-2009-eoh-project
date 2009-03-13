#include"tabs.h"
#include"../base/bang-module-registry.h"
#include<gtk/gtk.h>
#include<stdio.h>
#include<string.h>


void BMACHINE_open_module_tab(char *filename) {
	char *module_name;
	unsigned char *module_version;
	BANG_new_module(filename, &module_name,&module_version);
	
	//fprintf(stderr, "Module Path: %s name: %s version: %s\n",filename, module_name, module_version);
	
	/*
	char buff[256];	//figure out right size later
	strcat(buff,"Module name: ");
	strcat(buff,module_name);
	strcat(buff,"\tModule version: ");
	strcat(buff,(char*) module_version);
	
	
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),gtk_label_new(filename),gtk_label_new(buff));
	*/
	
	gtk_widget_show_all(notebook);
	filename[0] = '/';
}