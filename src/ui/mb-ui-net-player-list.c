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

#include "mb-ui-net-player-list.h"

#include <glib.h>
#include <glib-object.h>

#include <net/mb-net-holders.h>
typedef struct _Private {

	GtkListStore *list;
	MbNetClientGame *game;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_ui_net_player_list_get_property(GObject * object,
					       guint prop_id,
					       GValue * value,
					       GParamSpec * param_spec);
static void mb_ui_net_player_list_set_property(GObject * object,
					       guint prop_id,
					       const GValue * value,
					       GParamSpec * param_spec);


//static        guint   _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbUiNetPlayerList, mb_ui_net_player_list,
			G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_UI_TYPE_NET_PLAYER_LIST, Private))


static void _player_list_changed(MbNetClientGame * game,
				 MbUiNetPlayerList * self);

static void mb_ui_net_player_list_finalize(MbUiNetPlayerList * self);

static void mb_ui_net_player_list_init(MbUiNetPlayerList * self);
static void _stop(MbUiNetPlayerList * self);


static void mb_ui_net_player_list_init(MbUiNetPlayerList * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->list = gtk_list_store_new(1, G_TYPE_STRING);

}


static void mb_ui_net_player_list_finalize(MbUiNetPlayerList * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	_stop(self);
	g_object_unref(priv->list);
	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}

static gboolean _update_player_list(MbUiNetPlayerList * self)
{

	Private *priv;
	priv = GET_PRIVATE(self);
	gtk_list_store_clear(priv->list);
	GList *list = mb_net_client_game_get_players(priv->game);
	GList *next = list;
	g_print("player list !!! %d \n", g_list_length(list));
	while (next != NULL) {
		MbNetPlayerHolder *h;
		h = (MbNetPlayerHolder *) next->data;
		GtkTreeIter iter;
		GtkListStore *tree;


		tree = priv->list;

		gtk_list_store_append(tree, &iter);
		gtk_list_store_set(tree, &iter, 0, h->name, -1);

		next = g_list_next(next);

	}

	return FALSE;

}

static void _player_list_changed(MbNetClientGame * game,
				 MbUiNetPlayerList * self)
{
	g_idle_add((GSourceFunc) _update_player_list, self);
}

static gboolean _stop_idle(MbUiNetPlayerList * self)
{
	_stop(self);
	g_object_unref(self);
	return FALSE;
}

static void _stop_signal(MbNetClientGame * game, MbUiNetPlayerList * self)
{
	g_object_ref(self);
	g_idle_add((GSourceFunc) _stop_idle, self);

}

static void _stop(MbUiNetPlayerList * self)
{

	Private *priv;
	priv = GET_PRIVATE(self);
	if (priv->game != NULL) {
		g_signal_handlers_disconnect_by_func(priv->game,
						     _player_list_changed,
						     self);
		g_signal_handlers_disconnect_by_func(priv->game,
						     _stop_signal, self);
		g_object_unref(priv->game);
		priv->game = NULL;
	}
	gtk_list_store_clear(priv->list);

}



MbUiNetPlayerList *mb_ui_net_player_list_new(MbNetClientGame * game)
{
	MbUiNetPlayerList *self;

	self = MB_UI_NET_PLAYER_LIST(g_object_new
				     (MB_UI_TYPE_NET_PLAYER_LIST, NULL));

	return self;
}

void mb_ui_net_player_list_set_game(MbUiNetPlayerList * self,
				    MbNetClientGame * game)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	priv->game = game;
	g_object_ref(priv->game);
	g_signal_connect(priv->game, "player-list-changed",
			 (GCallback) _player_list_changed, self);
	g_signal_connect(priv->game, "stop",
			 (GCallback) _stop_signal, self);

	mb_net_client_game_ask_player_list(game);
}

GtkTreeModel *mb_ui_net_player_list_get_model(MbUiNetPlayerList * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	return GTK_TREE_MODEL(priv->list);
}



static void
mb_ui_net_player_list_get_property(GObject * object, guint prop_id,
				   GValue * value, GParamSpec * param_spec)
{
	MbUiNetPlayerList *self;

	self = MB_UI_NET_PLAYER_LIST(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_ui_net_player_list_set_property(GObject * object, guint prop_id,
				   const GValue * value,
				   GParamSpec * param_spec)
{
	MbUiNetPlayerList *self;

	self = MB_UI_NET_PLAYER_LIST(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_ui_net_player_list_class_init(MbUiNetPlayerListClass *
				 mb_ui_net_player_list_class)
{
	GObjectClass *g_object_class;

	parent_class =
	    g_type_class_peek_parent(mb_ui_net_player_list_class);


	g_type_class_add_private(mb_ui_net_player_list_class,
				 sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_ui_net_player_list_class);

	/* setting up property system */
	g_object_class->set_property = mb_ui_net_player_list_set_property;
	g_object_class->get_property = mb_ui_net_player_list_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_ui_net_player_list_finalize;


}
