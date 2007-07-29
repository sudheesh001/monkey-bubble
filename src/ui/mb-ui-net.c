/* this file is part of monkey-bubble
 *
 * AUTHORS
 *       Laurent Belminte        <laurent.belmonte@gmail.com>
 *
 * Copyright (C) 2007 Laurent Belmonte
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "mb-ui-net.h"


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <net/mb-net-server.h>
#include <net/mb-net-holders.h>
#include <net/mb-net-client-server.h>
#include <bonobo/bonobo-i18n.h>
#include <ui/mb-ui-net-player-list.h>

#include <ui/mb-ui-net-game-manager.h>
#include "ui-main.h"
typedef struct _Private {
	MbNetServer *server;
	MbNetClientServer *client;
	MbNetClientGame *game;
	MbUiNetGameManager *manager;
	GMutex *lock;
	GCond *cond;
	GtkWidget *go_button;
	GladeXML *glade_xml;
	GtkWidget *window;
	GtkLabel *connection_label;
	gchar *server_name;

	MbUiNetPlayerList *list;
	gboolean is_server;
	gboolean connected;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_ui_net_get_property(GObject * object,
				   guint prop_id,
				   GValue * value,
				   GParamSpec * param_spec);
static void mb_ui_net_set_property(GObject * object,
				   guint prop_id,
				   const GValue * value,
				   GParamSpec * param_spec);


//static        guint   _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbUiNet, mb_ui_net, G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_UI_TYPE_NET, Private))




static void mb_ui_net_finalize(MbUiNet * self);

static void mb_ui_net_init(MbUiNet * self);

static void _create_client_ui(MbUiNet * self);
static void _create_server_ui(MbUiNet * self);
static void _quit(MbUiNet * self);
static void _connect(MbUiNet * self);
static void _join_response(MbNetClientGame * game, gboolean ok,
			   MbUiNet * self);
static void _new_game_list(MbNetClientServer * client, MbUiNet * self);
static void _connected_and_create(MbNetClientServer * client, gboolean ok,
				  MbUiNet * self);
static void _connected_and_join(MbNetClientServer * client, gboolean ok,
				MbUiNet * self);
static void _game_created(MbNetClientServer * client,
			  MbNetClientGame * game, MbUiNet * self);
static void _stop_signal(MbNetClientGame * game, MbUiNet * self);
static void _set_sensitive(GtkWidget * w, gboolean s);
static void _set_status_message(MbUiNet * self, const gchar * message);

static void _disconnect(MbUiNet * self);
static void _server_disconnected(MbNetClientServer * server,
				 MbUiNet * self);
static void mb_ui_net_init(MbUiNet * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->server =
	    MB_NET_SERVER(g_object_new(MB_NET_TYPE_SERVER, NULL));
	priv->client =
	    MB_NET_CLIENT_SERVER(g_object_new
				 (MB_NET_TYPE_CLIENT_SERVER, NULL));
	priv->list = mb_ui_net_player_list_new();
	priv->manager =
	    MB_UI_NET_GAME_MANAGER(g_object_new
				   (MB_UI_TYPE_NET_GAME_MANAGER, NULL));
}


static void mb_ui_net_finalize(MbUiNet * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	_disconnect(self);
	g_object_unref(priv->game);
	g_object_unref(priv->client);

	g_object_unref(priv->list);
	if (priv->is_server == TRUE) {
		mb_net_server_stop(priv->server);
	}
	g_object_unref(priv->server);

	gtk_widget_destroy(priv->window);
	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}


MbUiNet *mb_ui_net_new()
{
	return MB_UI_NET(g_object_new(MB_UI_TYPE_NET, NULL));

}


void mb_ui_net_host(MbUiNet * self, GError ** error)
{
	GError *err = NULL;
	Private *priv;
	priv = GET_PRIVATE(self);

	mb_net_server_accept_on(priv->server, "mb://localhost:6666", &err);

	if (err != NULL) {
		g_propagate_error(error, err);
		return;
	}
	_create_server_ui(self);
	priv->is_server = TRUE;
	priv->server_name = g_strdup("localhost");
	_connect(self);
}

void mb_ui_net_connect(MbUiNet * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	_create_client_ui(self);
	priv->server = FALSE;
}





static void _disconnect(MbUiNet * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
//      _backtrace();//printstack();
	g_print("disconnect client ... \n");
	//if (priv->connected == TRUE) {

	g_signal_handlers_disconnect_by_func(priv->client,
					     _server_disconnected, self);
	mb_net_client_server_disconnect(priv->client);
	if (priv->game != NULL) {
		g_signal_handlers_disconnect_by_func(priv->game,
						     _join_response, self);
		g_signal_handlers_disconnect_by_func(priv->game,
						     _stop_signal, self);
		g_object_unref(priv->game);
		priv->game = NULL;
	}

	g_signal_handlers_disconnect_by_func(priv->client,
					     _connected_and_join, self);
	g_signal_handlers_disconnect_by_func(priv->client,
					     _connected_and_create, self);
	g_signal_handlers_disconnect_by_func(priv->client,
					     _new_game_list, self);


	gtk_label_set_label(priv->connection_label,
			    g_strdup_printf("", priv->server_name));
//      }

	if (priv->is_server == FALSE) {
		gtk_widget_set_sensitive(glade_xml_get_widget
					 (priv->glade_xml, "connect_hbox"),
					 TRUE);
	}
}


static void _stop_signal(MbNetClientGame * game, MbUiNet * self)
{
	_disconnect(self);

}

static void _server_disconnected(MbNetClientServer * server,
				 MbUiNet * self)
{

	g_print("disconnect client... MB_UI_NET ..\n");
	_disconnect(self);
}

static void _connect_server(MbUiNet * self)
{

	GtkWidget *entry;

	Private *priv;
	priv = GET_PRIVATE(self);


	entry = glade_xml_get_widget(priv->glade_xml, "server_name_entry");
	gchar *name = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	priv->server_name = name;

	_connect(self);
}

static void _game_start(MbNetClientGame * game, MbNetClientMatch * match,
			MbUiNet * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	gtk_widget_destroy(priv->window);
	g_signal_handlers_disconnect_matched(game, G_SIGNAL_MATCH_DATA, 0,
					     0, NULL, NULL, self);
	g_signal_handlers_disconnect_matched(priv->client,
					     G_SIGNAL_MATCH_DATA, 0, 0,
					     NULL, NULL, self);


}

static void _join_response(MbNetClientGame * game, gboolean ok,
			   MbUiNet * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_print("receive join response ... \n");
	if (ok == FALSE) {
		//g_object_ref(self);
		//g_idle_add((GSourceFunc) _disconnect_idle, self);
		_disconnect(self);
		_set_status_message(self,
				    g_strdup_printf("Can't join game ...",
						    priv->server_name));

	} else {
		if (mb_net_client_game_is_master(game)) {
			_set_sensitive(priv->go_button, TRUE);
		} else {
			_set_sensitive(priv->go_button, FALSE);
		}
		_set_status_message(self,
				    g_strdup_printf("Connected to %s",
						    GET_PRIVATE(self)->
						    server_name));
		mb_ui_net_game_manager_set_game(priv->manager, priv->game);
		mb_ui_net_game_manager_set_client(priv->manager,
						  priv->client);
		if (priv->is_server == TRUE) {
			mb_ui_net_game_manager_set_server(priv->manager,
							  priv->server);
		}
		g_signal_connect(priv->game, "stop",
				 (GCallback) _stop_signal, self);

		g_signal_connect(priv->game, "start",
				 (GCallback) _game_start, self);

		mb_ui_net_player_list_set_game(priv->list, priv->game);

		UiMain *ui;

		ui = ui_main_get_instance();
		ui_main_set_game_manager(ui, GAME_MANAGER(priv->manager));


	}

}

static void _game_created(MbNetClientServer * client,
			  MbNetClientGame * game, MbUiNet * self)
{

	Private *priv;
	priv = GET_PRIVATE(self);
	g_object_ref(game);
	priv->game = game;
	g_signal_connect(priv->game, "join-response",
			 (GCallback) _join_response, self);
	mb_net_client_game_join(game);
}

static void _connected_and_create(MbNetClientServer * client, gboolean ok,
				  MbUiNet * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_signal_connect(priv->client, "game-created",
			 (GCallback) _game_created, self);

	mb_net_client_server_create_game(priv->client, "bubble game",
					 NULL);
}



static void _new_game_list(MbNetClientServer * client, MbUiNet * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	GList *games = mb_net_client_server_get_games(client);
	if (g_list_length(games) == 1) {

		int game_id =
		    ((MbNetSimpleGameHolder *) games->data)->handler_id;
		priv->game =
		    mb_net_client_server_create_client(client, game_id);
		g_signal_connect(priv->game, "join-response",
				 (GCallback) _join_response, self);

		mb_net_client_game_join(priv->game);
	} else {
		//g_object_ref(self);
		_disconnect(self);
//              g_idle_add((GSourceFunc) _disconnect_idle, self);
		_set_status_message(self,
				    g_strdup_printf("No game found ...",
						    priv->server_name));

	}
}

static void _connected_and_join(MbNetClientServer * client, gboolean ok,
				MbUiNet * self)
{
	GtkWidget *entry;

	Private *priv;
	priv = GET_PRIVATE(self);

	g_signal_connect(priv->client, "new-game-list",
			 (GCallback) _new_game_list, self);
	mb_net_client_server_ask_games(priv->client, NULL);

}
static void _connect(MbUiNet * self)
{
	GtkWidget *entry;

	Private *priv;
	priv = GET_PRIVATE(self);

	priv->connected = TRUE;

	if (priv->server == FALSE) {
		GtkWidget *entry;
		entry =
		    glade_xml_get_widget(priv->glade_xml,
					 "server_name_entry");
		priv->server_name =
		    g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

	}
	_set_status_message(self, g_strdup_printf("Connecting %s ...",
						  priv->server_name));


	if (priv->server == FALSE) {
		_set_sensitive(glade_xml_get_widget
			       (priv->glade_xml, "connect_hbox"), FALSE);
	}
	mb_net_client_server_set_name(priv->client, g_get_user_name());
	GError *error;
	error = NULL;
	if (priv->is_server == TRUE) {
		g_signal_connect(priv->client, "connected",
				 (GCallback) _connected_and_create, self);
	} else {
		g_signal_connect(priv->client, "connected",
				 (GCallback) _connected_and_join, self);

	}

	g_signal_connect(priv->client, "disconnected",
			 (GCallback) _server_disconnected, self);
	mb_net_client_server_connect(priv->client, priv->server_name,
				     &error);

	if (error != NULL) {
		g_signal_handlers_disconnect_by_func(priv->client,
						     _connected_and_join,
						     self);
		g_signal_handlers_disconnect_by_func(priv->client,
						     _connected_and_create,
						     self);

		_set_status_message(self,
				    g_strdup_printf
				    ("Can't connect to %s ...",
				     priv->server_name));

		if (priv->is_server == FALSE) {
			_set_sensitive(glade_xml_get_widget
				       (priv->glade_xml, "connect_hbox"),
				       TRUE);
		}

	}

}

static void _start_signal(MbUiNet * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	mb_net_client_game_start(priv->game);
}

static void _create_basic_ui(MbUiNet * self, const gchar * path)
{
	GtkWidget *item;

	GtkTreeViewColumn *column;

	Private *priv;
	priv = GET_PRIVATE(self);


	priv->glade_xml = glade_xml_new(path, "network_window", NULL);

	priv->window =
	    glade_xml_get_widget(priv->glade_xml, "network_window");


	priv->connection_label =
	    GTK_LABEL(glade_xml_get_widget
		      (priv->glade_xml, "connection_state_label"));

	priv->go_button =
	    glade_xml_get_widget(priv->glade_xml, "start_button");
	g_signal_connect_swapped(priv->go_button, "clicked",
				 GTK_SIGNAL_FUNC(_start_signal), self);


	item = glade_xml_get_widget(priv->glade_xml, "quit_button");
	g_signal_connect_swapped(item, "clicked", GTK_SIGNAL_FUNC(_quit),
				 self);

	item = glade_xml_get_widget(priv->glade_xml, "network_window");
	g_signal_connect_swapped(item, "delete_event",
				 GTK_SIGNAL_FUNC(_quit), self);

	item = glade_xml_get_widget(priv->glade_xml, "players_treeview");

	column =
	    gtk_tree_view_column_new_with_attributes(_("_Player name"),
						     gtk_cell_renderer_text_new
						     (), "text", 0,
						     (char *) NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(item), column);

	gtk_tree_view_set_model(GTK_TREE_VIEW(item),
				mb_ui_net_player_list_get_model(priv->
								list));


}

static void _create_server_ui(MbUiNet * self)
{
	_create_basic_ui(self,
			 DATADIR "/monkey-bubble/glade/netserver.glade");
}

static void _create_client_ui(MbUiNet * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	GtkWidget *item;
	GtkTreeViewColumn *column;

	_create_basic_ui(self,
			 DATADIR "/monkey-bubble/glade/netgame.glade");


	item = glade_xml_get_widget(priv->glade_xml, "go_button");
	g_signal_connect_swapped(item, "clicked", (GCallback) _connect,
				 self);


}


static gboolean set_sensitive_true_idle(gpointer data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(data), TRUE);

	return FALSE;

}

static gboolean set_sensitive_false_idle(gpointer data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(data), FALSE);

	return FALSE;

}

static void _set_sensitive(GtkWidget * w, gboolean s)
{
	if (s) {
		g_idle_add(set_sensitive_true_idle, w);
	} else {
		g_idle_add(set_sensitive_false_idle, w);
	}
}



struct StatusJob {
	MbUiNet *self;
	gchar *message;
} StatusJob;


static gboolean _status_idle(gpointer data)
{
	struct StatusJob *j;
	MbUiNet *self;

	j = (struct StatusJob *) data;

	self = j->self;

	gtk_label_set_label(GET_PRIVATE(self)->connection_label,
			    j->message);

	g_free(j->message);
	g_free(j);
	return FALSE;
}



static void _set_status_message(MbUiNet * self, const gchar * message)
{

	struct StatusJob *j;
	j = (struct StatusJob *) g_malloc(sizeof(StatusJob));
	j->self = self;
	j->message = g_strdup(message);
	g_idle_add(_status_idle, j);
}


static void _quit(MbUiNet * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	_disconnect(self);
	g_object_unref(self);
}

static void
mb_ui_net_get_property(GObject * object, guint prop_id, GValue * value,
		       GParamSpec * param_spec)
{
	MbUiNet *self;

	self = MB_UI_NET(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_ui_net_set_property(GObject * object, guint prop_id,
		       const GValue * value, GParamSpec * param_spec)
{
	MbUiNet *self;

	self = MB_UI_NET(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void mb_ui_net_class_init(MbUiNetClass * mb_ui_net_class)
{
	GObjectClass *g_object_class;

	parent_class = g_type_class_peek_parent(mb_ui_net_class);


	g_type_class_add_private(mb_ui_net_class, sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_ui_net_class);

	/* setting up property system */
	g_object_class->set_property = mb_ui_net_set_property;
	g_object_class->get_property = mb_ui_net_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_ui_net_finalize;


}
