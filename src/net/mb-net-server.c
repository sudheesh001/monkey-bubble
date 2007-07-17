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
#include <net/mb-net-handler-manager.h>
#include <net/mb-net-server-handler.h>
#include <glib.h>
#include <glib-object.h>


typedef struct _Private {
	MbNetConnection *main_connection;
	GList *connections;
	GMutex *lock;

	MbNetServerHandler *main_handler;
	MbNetHandlerManager *manager;


	GList *games;
	GMutex *games_lock;

	GList *players;
	GMutex *players_lock;

	guint32 current_player_id;
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


static void _player_disconnected(MbNetServer * self,
				 MbNetServerPlayer * p);
static void _disconnected(MbNetServer * self, MbNetConnection * con);
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
			 guint32 handler_id, guint32 player_id,
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

	priv->lock = g_mutex_new();
	priv->players_lock = g_mutex_new();
	priv->games_lock = g_mutex_new();
	priv->manager =
	    MB_NET_HANDLER_MANAGER(g_object_new
				   (MB_NET_TYPE_HANDLER_MANAGER, NULL));
	mb_net_handler_manager_register(priv->manager,
					MB_NET_HANDLER(priv->
						       main_handler));
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

	g_mutex_lock(priv->lock);
	GList *next = priv->connections;
	while (next != NULL) {
		MbNetConnection *con = MB_NET_CONNECTION(next->data);
		g_signal_handlers_disconnect_by_func(con, (GCallback)
						     _receive_message,
						     self);
		g_signal_handlers_disconnect_by_func(con, (GCallback)
						     _disconnected, self);
		next = g_list_next(next);
	}

	g_list_foreach(priv->connections, (GFunc) mb_net_connection_stop,
		       NULL);
	g_list_foreach(priv->connections, (GFunc) g_object_unref, NULL);
	g_list_free(priv->connections);
	mb_net_connection_stop(priv->main_connection, NULL);
	g_object_unref(priv->main_connection);
	priv->main_connection = NULL;
	g_mutex_unlock(priv->lock);
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

	mb_net_handler_manager_message(priv->manager, con, src_handler_id,
				       dst_handler_id, action_id, m);
}

static void _disconnected(MbNetServer * self, MbNetConnection * con)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->lock);
	priv->connections = g_list_remove(priv->connections, con);
	g_signal_handlers_disconnect_by_func(con,
					     (GCallback) _receive_message,
					     self);
	g_signal_handlers_disconnect_by_func(con,
					     (GCallback) _disconnected,
					     self);

	g_mutex_unlock(priv->lock);
	g_object_unref(con);
}

static void _new_connection(MbNetServer * self, MbNetConnection * con)
{

	if (con != NULL) {
		Private *priv;
		priv = GET_PRIVATE(self);
		g_object_ref(con);

		g_mutex_lock(priv->lock);
		priv->connections = g_list_prepend(priv->connections, con);

		g_signal_connect_swapped(con, "receive-message",
					 (GCallback) _receive_message,
					 self);
		g_signal_connect_swapped(con, "disconnected",
					 (GCallback) _disconnected, self);
		mb_net_connection_listen(con, NULL);
		g_mutex_unlock(priv->lock);
	}

}

static MbNetServerPlayer *_find_by_id(MbNetServer * self, guint32 id)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_mutex_lock(priv->players_lock);
	MbNetServerPlayer *ret = NULL;
	GList *next = priv->players;
	while (next != NULL) {
		MbNetServerPlayer *p = MB_NET_SERVER_PLAYER(next->data);
		if (mb_net_server_player_get_id(p) == id) {
			ret = p;
			break;
		}
		next = g_list_next(next);
	}
	g_mutex_unlock(priv->players_lock);

	if (ret == NULL) {
		g_print("player(%d) not found\n", id);
	}

	return ret;
}

MbNetServerPlayer *mb_net_server_get_player(MbNetServer * self,
					    guint32 player_id)
{
	return _find_by_id(self, player_id);
}


static void _remove_player(MbNetServer * self, MbNetServerPlayer * p)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->players_lock);
	g_signal_handlers_disconnect_by_func(p, (GCallback)
					     _player_disconnected, self);
	priv->players = g_list_remove(priv->players, p);
	g_object_unref(p);
	g_mutex_unlock(priv->players_lock);
}


static void _player_disconnected(MbNetServer * self, MbNetServerPlayer * p)
{
	_remove_player(self, p);
}

static void _ask_register_player(MbNetServer * self, MbNetConnection * con,
				 guint32 handler_id,
				 MbNetPlayerHolder * holder,
				 MbNetServerHandler * handler)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	MbNetServerPlayer *p;
	guint32 player_id = priv->current_player_id++;
	p = mb_net_server_player_new(con, player_id, holder->name);
	g_mutex_lock(priv->players_lock);
	g_signal_connect_swapped(p, "disconnected",
				 (GCallback) _player_disconnected, self);
	priv->players = g_list_prepend(priv->players, p);
	holder->player_id = player_id;

	g_mutex_unlock(priv->players_lock);
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

static void _game_stopped(MbNetServer * self, MbNetGame * game)
{

	Private *priv;
	priv = GET_PRIVATE(self);
	g_mutex_lock(priv->games_lock);
	g_signal_connect_swapped(game, "stopped",
				 (GCallback) _game_stopped, self);

	priv->games = g_list_remove(priv->games, game);
	g_signal_handlers_disconnect_by_func(game,
					     (GCallback) _game_stopped,
					     self);

	g_mutex_unlock(priv->games_lock);
	g_object_unref(game);

}

static void _create_game(MbNetServer * self, MbNetConnection * con,
			 guint32 handler_id, guint32 player_id,
			 const gchar * name, MbNetServerHandler * handler)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	MbNetServerPlayer *p = _find_by_id(self, player_id);
	if (p == NULL) {
		mb_net_server_handler_send_create_game_response(handler,
								con,
								handler_id,
								0);
	} else {

		MbNetGame *game =
		    mb_net_game_new(name, p, priv->manager, self);

		g_mutex_lock(priv->games_lock);
		g_signal_connect_swapped(game, "stopped",
					 (GCallback) _game_stopped, self);

		priv->games = g_list_prepend(priv->games, game);

		g_mutex_unlock(priv->games_lock);
		mb_net_server_handler_send_create_game_response(handler,
								con,
								handler_id,
								game->info.
								handler_id);
	}

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
