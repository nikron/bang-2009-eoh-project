#include<gtk/gtk.h>
#include<stdio.h>

static const GtkActionEntry entries[] =
{
	{ "FileMenuAction", NULL, "_File", "", "Access external files relevant to this program.", NULL },
	{ "OpenModuleAction", GTK_STOCK_OPEN, "_Open Module", "<control>o", "Open a module.", NULL },

	{ "EditMenuAction", NULL, "_Edit", "", "Edit functionality.", NULL },
	{ "EditPreferencesAction", GTK_STOCK_PREFERENCES, "_Edit Prefrences", "<control>e", "Edit your prefrences.", NULL },

	{ "ServerMenuAction", NULL, "_Server", "", "Control the bang server.", NULL },
	{ "StartStopServerAction", GTK_STOCK_CONNECT, "_Start Server", "<control>s", "Start or stop the the server.", NULL }
};

static const guint n_entries = G_N_ELEMENTS(entries);

GtkUIManager* BMACHINE_create_menus(GtkWidget *vbox, GtkWidget *window) {

	GtkActionGroup *act_grp = gtk_action_group_new("MenuBarAactions");
	gtk_action_group_add_actions(act_grp,entries,n_entries,NULL);

	GtkUIManager *m_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(m_manager,act_grp,0);

	GError *err = NULL;
	gtk_ui_manager_add_ui_from_file(m_manager,"menus.xml",&err);

	/* TODO: make this do something more useful. */
	if (err)
		printf("%s\n",err->message);

	GtkWidget *menu_bar = gtk_ui_manager_get_widget(m_manager,"/MainMenu");
	gtk_box_pack_start(GTK_BOX(vbox),menu_bar,FALSE,FALSE,0);

	gtk_window_add_accel_group(GTK_WINDOW(window),gtk_ui_manager_get_accel_group(m_manager));

	return m_manager;
}
