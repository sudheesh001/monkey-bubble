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

#include "mb-net-server.h"
#include <net/mb-net-connection.h>
#include <net/mb-net-handler.h>
#include <net/mb-net-server-handler.h>
#include <glib.h>
#include <glib-object.h>


typedef struct _Private {
	MbNetConnection *main_connection;
	GList *connections;
	MbNetServerHandler *main_handler;

	GList *games;
	GList *players;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	NEW_PLAYER,
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_server_get_property(GObject * object,
				       guint prop_id,
				       GValue * value,
				       GParamSpec * param_spec);
static void mb_net_server_set_property(GObject * object,
				       guint prop_id,
				       const GValue * value,
				       GParamSpec * param_spec);


static guint _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetServer, mb_net_server, G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_SERVER, Private))




static void _new_connection(MbNetServer * self, MbNetConnection * con);
static void _receive_message(MbNetServer * self, MbNetMessage * m,
			     MbNetConnection * con);
static void mb_net_server_finalize(MbNetServer * self);

static void mb_net_server_init(MbNetServer * self);

static void _ask_game_list(MbNetServer * self, MbNetConnection * con,
			   guint32 handler_id,
			   MbNetServerHandler * handler);
static void _ask_register_player(MbNetServer * self, MbNetConnection * con,
				 guint32 handler_id,
				 MbNetPlayerHolder * holder,
				 MbNetServerHandler * handler);

static void _create_game(MbNetServer * self, MbNetConnection * con,
			 guint32 handler_id,
			 const gchar * name, MbNetServerHandler * handler);
static void mb_net_server_init(MbNetServer * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->main_connection =
	    MB_NET_CONNECTION(g_object_new(MB_NET_TYPE_CONNECTION, NULL));

	g_signal_connect_swapped(priv->main_connection, "new-connection",
				 (GCallback) _new_connection, self);
	priv->main_handler =
	    MB_NET_SERVER_HANDLER(g_object_new
				  (MB_NET_TYPE_SERVER_HANDLER, NULL));

	g_signal_connect_swapped(priv->main_handler, "ask-game-list",
				 (GCallback) _ask_game_list, self);


	g_signal_connect_swapped(priv->main_handler, "ask-register-player",
				 (GCallback) _ask_register_player, self);


	g_signal_connect_swapped(priv->main_handler, "create-game",
				 (GCallback) _create_game, self);


}


static void mb_net_server_finalize(MbNetServer * self)
{

	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}

void mb_net_server_stop(MbNetServer * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);


	g_list_foreach(priv->connections, (GFunc) mb_net_connection_stop,
		       NULL);
	g_list_foreach(priv->connections, (GFunc) g_object_unref, NULL);
	g_list_free(priv->connections);
	mb_net_connection_stop(priv->main_connection, NULL);
	g_object_unref(priv->main_connection);
	priv->main_connection = NULL;
}



void
mb_net_server_accept_on(MbNetServer * self, const gchar * uri,
			GError ** error)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	mb_net_connection_accept_on(priv->main_connection, uri, error);
}



static void
_receive_message(MbNetServer * self, MbNetMessage * m,
		 MbNetConnection * con)
{

	Private *priv;
	priv = GET_PRIVATE(self);

	guint32 src_handler_id, dst_handler_id, action_id;
	mb_net_message_read_init(m, &src_handler_id, &dst_handler_id,
				 &action_id);

	if (dst_handler_id == 0) {
		mb_net_handler_receive(MB_NET_HANDLER(priv->main_handler),
				       con, src_handler_id, dst_handler_id,
				       action_id, m);
	} else {
		/*
		   g_print("recevie .. \n");
		   mb_net_handler_receive(MB_NET_HANDLER(priv->main_handler),
		   con, src_handler_id, dst_handler_id,
		   action_id, m); */
	}
}

static void _new_connection(MbNetServer * self, MbNetConnection * con)
{

	if (con != NULL) {
		Private *priv;
		priv = GET_PRIVATE(self);
		g_object_ref(con);

		priv->connections = g_list_prepend(priv->connections, con);

		g_signal_connect_swapped(con, "receive-message",
					 (GCallback) _receive_message,
					 self);
		mb_net_connection_listen(con, NULL);
	}

}

static MbNetServerPlayer *_create_player(MbNetConnection * con,
					 guint32 handler_id,
					 const gchar * name)
{
	MbNetServerPlayer *p;
	p = g_new0(MbNetServerPlayer, 1);
	p->con = con;
	p->handler_id = handler_id;
	p->name = g_strdup(name);
	return p;
}

static void _ask_register_player(MbNetServer * self, MbNetConnection * con,
				 guint32 handler_id,
				 MbNetPlayerHolder * holder,
				 MbNetServerHandler * handler)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	MbNetServerPlayer *p;
	p = _create_player(con, handler_id, holder->name);
	priv->players = g_list_prepend(priv->players, p);
	mb_net_server_handler_send_register_player_response(handler, con,
							    handler_id,
							    holder, TRUE);
	g_signal_emit(self, _signals[NEW_PLAYER], 0, p);
}


void
_ask_game_list(MbNetServer * self, MbNetConnection * con,
	       guint32 handler_id, MbNetServerHandler * handler)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	MbNetGameListHolder *holder;
	holder = g_new0(MbNetGameListHolder, 1);
	GList *l = NULL;
	GList *next = priv->games;
	for (next = priv->games; next != NULL; next = g_list_next(next)) {
		MbNetGame *g = MB_NET_GAME(next->data);

		l = g_list_prepend(l,
				   mb_net_simple_game_holder_create(g->
								    info.
								    handler_id,
								    g->
								    info.
								    name));
	}

	holder->games = l;
	mb_net_server_handler_send_game_list(handler, con, handler_id,
					     holder);
}

static void _create_game(MbNetServer * self, MbNetConnection * con,
			 guint32 handler_id,
			 const gchar * name, MbNetServerHandler * handler)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	MbNetGame *game = mb_net_game_new(name, handler_id);
	game->info.handler_id = 1;
	priv->games = g_list_prepend(priv->games, game);
	mb_net_server_handler_send_create_game_response(handler, con,
							handler_id, 1);

}


static void
mb_net_server_get_property(GObject * object, guint prop_id, GValue * value,
			   GParamSpec * param_spec)
{
	MbNetServer *self;

	self = MB_NET_SERVER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_server_set_property(GObject * object, guint prop_id,
			   const GValue * value, GParamSpec * param_spec)
{
	MbNetServer *self;

	self = MB_NET_SERVER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_server_class_init(MbNetServerClass * mb_net_server_class)
{
	GObjectClass *g_object_class;

	parent_class = g_type_class_peek_parent(mb_net_server_class);


	g_type_class_add_private(mb_net_server_class, sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_server_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_server_set_property;
	g_object_class->get_property = mb_net_server_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_server_finalize;

	_signals[NEW_PLAYER] =
	    g_signal_new("new-player", MB_NET_TYPE_SERVER,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetServerClass,
					 new_player), NULL, NULL,
			 g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
			 G_TYPE_POINTER);

}
