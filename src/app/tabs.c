#include"tabs.h"
#include"../base/bang-module-registry.h"
#include"../base/bang-signals.h"
#include<gtk/gtk.h>
#include<stdlib.h>
#include<string.h>

void update_peer_list(int signal, int num_args, void **args);

static void add_peer_to_list(int peer_id);

static void remove_peer_from_list(int peer_id);


void BMACHINE_tabs_init() {
	
	BANG_install_sighandler(BANG_PEER_ADDED,&update_peer_list);
	BANG_install_sighandler(BANG_PEER_REMOVED,&update_peer_list);
}

void BMACHINE_open_module_tab(char *filename) {
	char *module_name;
	unsigned char *module_version;
	BANG_new_module(filename, &module_name,&module_version);
	
	
	/*
	fprintf(stderr, "HEY HEY LOOK AT ME: Module Path: %s name: %s version: %s\n",filename, module_name, module_version);
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

void update_peer_list(int signal, int num_args, void **args) {
	gdk_threads_enter();
	
	switch (signal) {
		case BANG_PEER_ADDED:
			add_peer_to_list(*((int*)args));
			break;
		case BANG_PEER_REMOVED:
			remove_peer_from_list(*((int*)args));
			break;
	}
	
	
	
	gdk_threads_leave();
	
	int i = 0;
	for (i = 0; i < num_args; ++i) {
		free(args[i]);
	}

	free(args);
}

static void add_peer_to_list(int peer_id) {
	char peer_id_buf[100];	// probably big enough for most peer numbers, yeah? It's a maaaaaagic number.
	sprintf(peer_id_buf, "%d", peer_id);
	gtk_container_add(GTK_CONTAINER(peerlist),gtk_label_new(peer_id_buf));
}

static void remove_peer_from_list(int peer_id) {
	char peer_id_buf[100];	// probably big enough for most peer numbers, yeah? It's a maaaaaagic number.
	sprintf(peer_id_buf, "%d", peer_id);
	
	GList *peerlist_elts = gtk_container_get_children(GTK_CONTAINER(peerlist));
	GList *iter = peerlist_elts;
	
	while (iter)
	{
		if (strcmp(gtk_label_get_text(iter->data),peer_id_buf) == 0)
			break;
		else
			iter = iter->next;
	}
	
	gtk_container_remove(GTK_CONTAINER(peerlist),iter->data);
	
	peer_id = peer_id;
}