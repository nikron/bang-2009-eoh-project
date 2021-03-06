#include"server-menu.h"
#include"file-menu.h"
#include<gtk/gtk.h>
#include<stdio.h>

static const GtkActionEntry entries[] =
{
	{ "FileMenuAction", NULL, "_File", "", "Access external files relevant to this program.", NULL },
	{ "OpenModuleAction", GTK_STOCK_OPEN, "_Open Module", "<control>o", "Open a module.", G_CALLBACK(BMACHINE_open_module) },
	{ "CloseModuleAction", GTK_STOCK_CLOSE, "C_lose Module", "<control>l", "Close a module.", NULL },
	{ "ExitAction", GTK_STOCK_QUIT, "E_xit", "<control>x", "Exit the program.", G_CALLBACK(BMACHINE_exit) },

	{ "EditMenuAction", NULL, "_Edit", "", "Edit functionality.", NULL },
	{ "EditPreferencesAction", GTK_STOCK_PREFERENCES, "_Edit Prefrences", "<control>e", "Edit your prefrences.", NULL },

	{ "NetworkMenuAction", NULL, "_Network", "", "Control the bang networking.", NULL },
	{ "StartStopServerAction", GTK_STOCK_CONNECT, "_Start Server", "<control>s", "Start or stop the the server.", G_CALLBACK(BMACHINE_change_server_status) },
	{ "ConnectPeerAction", GTK_STOCK_CONNECT, "Connect to _Peer", "<control>p", "Connect to a peer.", G_CALLBACK(BMACHINE_connect_peer) },
	{ "ScanAction", GTK_STOCK_NETWORK, "S_can", "<control>c", "Scan for peers.", NULL }
};

static const guint n_entries = G_N_ELEMENTS(entries);

GtkUIManager* BMACHINE_create_menus(GtkWidget *vbox, GtkWidget *window, GtkWidget *notebook) {

	GtkActionGroup *act_grp = gtk_action_group_new("MenuBarAactions");
	gtk_action_group_add_actions(act_grp,entries,n_entries,NULL);

	GtkUIManager *m_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(m_manager,act_grp,0);

	GError *err = NULL;
	gtk_ui_manager_add_ui_from_file(m_manager,"menus.xml",&err);

	/* TODO: make this do something more useful. */
	if (err)
		printf("%s\n",err->message);

	GtkWidget *ssserver = gtk_ui_manager_get_widget(m_manager,"/MainMenu/NetworkMenu/SSServer");
	BMACHINE_setup_server_menu(ssserver);
	GtkWidget *menu_bar = gtk_ui_manager_get_widget(m_manager,"/MainMenu");
	gtk_box_pack_start(GTK_BOX(vbox),menu_bar,FALSE,FALSE,0);

	gtk_window_add_accel_group(GTK_WINDOW(window),gtk_ui_manager_get_accel_group(m_manager));

	BMACHINE_create_file_menu(notebook);

	return m_manager;
}
