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

#include "mb-net-client-server.h"

#include <glib.h>
#include <glib-object.h>
#include <net/mb-net-connection.h>
#include <net/mb-net-server-handler.h>
#include <net/mb-net-handler-manager.h>

typedef struct _Private {
	MbNetConnection *con;
	MbNetServerHandler *handler;
	gchar *name;
	MbNetHandlerManager *manager;
	gboolean connected;
	gboolean registred;
	GMutex *games_lock;
	GList *games;
	MbNetClientGame *game;
	guint32 player_id;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	CONNECTED,
	NEW_GAME_LIST,
	GAME_CREATED,
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_client_server_get_property(GObject * object,
					      guint prop_id,
					      GValue * value,
					      GParamSpec * param_spec);
static void mb_net_client_server_set_property(GObject * object,
					      guint prop_id,
					      const GValue * value,
					      GParamSpec * param_spec);


static guint _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetClientServer, mb_net_client_server,
			G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_CLIENT_SERVER, Private))




static void mb_net_client_server_finalize(MbNetClientServer * self);

static void mb_net_client_server_init(MbNetClientServer * self);

static void _game_list(MbNetClientServer * self,
		       MbNetConnection * con, guint32 handler_id,
		       MbNetGameListHolder * holder,
		       MbNetServerHandler * handler);

static void
_register_player_response(MbNetClientServer * self,
			  MbNetConnection * con,
			  guint32 handler_id,
			  MbNetPlayerHolder * holder,
			  gboolean ok, MbNetServerHandler * handler);

static void _create_game_response(MbNetClientServer * self,
				  MbNetConnection * con,
				  guint32 handler_id, guint32 game_id,
				  MbNetServerHandler * handler);
static void _message(MbNetClientServer * self, MbNetMessage * m,
		     MbNetConnection * con);



static void mb_net_client_server_init(MbNetClientServer * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);


	priv->con =
	    MB_NET_CONNECTION(g_object_new(MB_NET_TYPE_CONNECTION, NULL));
	priv->handler =
	    MB_NET_SERVER_HANDLER(g_object_new
				  (MB_NET_TYPE_SERVER_HANDLER, NULL));

	g_signal_connect_swapped(priv->con, "receive-message",
				 (GCallback) _message, self);

	g_signal_connect_swapped(priv->handler, "register-player-response",
				 (GCallback) _register_player_response,
				 self);

	g_signal_connect_swapped(priv->handler, "game-list",
				 (GCallback) _game_list, self);
	g_signal_connect_swapped(priv->handler, "create-game-response",
				 (GCallback) _create_game_response, self);
	priv->games_lock = g_mutex_new();
	priv->manager =
	    MB_NET_HANDLER_MANAGER(g_object_new
				   (MB_NET_TYPE_HANDLER_MANAGER, NULL));
	mb_net_handler_manager_register(priv->manager,
					MB_NET_HANDLER(priv->handler));
}


static void mb_net_client_server_finalize(MbNetClientServer * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}


void mb_net_client_server_set_name(MbNetClientServer * self,
				   const gchar * name)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->name = g_strdup(name);
}

void mb_net_client_server_connect(MbNetClientServer * self,
				  const gchar * uri, GError ** error)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	GError *err;
	g_assert(priv->name != NULL);
	err = NULL;
	mb_net_connection_connect(priv->con, "mb://localhost:6666", &err);
	if (err != NULL) {
		g_propagate_error(error, err);
		return;
	}

	mb_net_connection_listen(priv->con, &err);
	if (err != NULL) {
		g_propagate_error(error, err);
		return;
	}


	priv->connected = TRUE;


	MbNetPlayerHolder *holder =
	    mb_net_player_holder_create(priv->name);

	mb_net_server_handler_send_ask_register_player(priv->handler,
						       priv->con, 0,
						       holder);


}


void mb_net_client_server_disconnect(MbNetClientServer * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_assert(priv->connected);
	mb_net_connection_stop(priv->con, NULL);
	priv->registred = FALSE;
	priv->connected = FALSE;
	priv->player_id = 0;
}

static void _copy_holder(MbNetSimpleGameHolder * h,
			 MbNetClientServer * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	priv->games =
	    g_list_append(priv->games,
			  mb_net_simple_game_holder_create(h->handler_id,
							   h->name));

}

static void _game_list(MbNetClientServer * self,
		       MbNetConnection * con, guint32 handler_id,
		       MbNetGameListHolder * holder,
		       MbNetServerHandler * handler)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->games_lock);

	g_list_foreach(priv->games, (GFunc) mb_net_simple_game_holder_free,
		       NULL);
	g_list_free(priv->games);
	priv->games = NULL;


	g_list_foreach(holder->games, (GFunc) _copy_holder, self);
	g_mutex_unlock(priv->games_lock);

	g_signal_emit(self, _signals[NEW_GAME_LIST], 0);
}


void mb_net_client_server_ask_games(MbNetClientServer * self,
				    GError ** error)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_assert(priv->registred);

	mb_net_server_handler_send_ask_game_list(priv->handler, priv->con,
						 0);

}

static void _create_game_response(MbNetClientServer * self,
				  MbNetConnection * con,
				  guint32 handler_id, guint32 game_id,
				  MbNetServerHandler * handler)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	MbNetClientGame *game;
	game =
	    mb_net_client_game_create(game_id, priv->player_id, priv->con,
				      priv->manager);
	g_signal_emit(self, _signals[GAME_CREATED], 0, game);
	priv->game = game;
}


MbNetClientGame *mb_net_client_server_create_client(MbNetClientServer *
						    self, guint32 game_id)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	MbNetClientGame *game;
	game =
	    mb_net_client_game_create(game_id, priv->player_id, priv->con,
				      priv->manager);
	priv->game = game;
	return game;
}

void mb_net_client_server_create_game(MbNetClientServer * self,
				      const gchar * name, GError ** error)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_assert(priv->registred);

	mb_net_server_handler_send_create_game(priv->handler, priv->con, 0,
					       priv->player_id, name);
}


static void
_message(MbNetClientServer * self, MbNetMessage * m, MbNetConnection * con)
{

	Private *priv;
	priv = GET_PRIVATE(self);

	guint32 src_handler_id, dst_handler_id, action_id;
	mb_net_message_read_init(m, &src_handler_id, &dst_handler_id,
				 &action_id);

	mb_net_handler_manager_message(priv->manager, con, src_handler_id,
				       dst_handler_id, action_id, m);

}

static void
_register_player_response(MbNetClientServer * self,
			  MbNetConnection * con,
			  guint32 handler_id,
			  MbNetPlayerHolder * holder,
			  gboolean ok, MbNetServerHandler * handler)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	if (ok != TRUE) {
		mb_net_connection_stop(priv->con, NULL);
		priv->connected = FALSE;
		priv->registred = FALSE;
	} else {
		priv->connected = TRUE;
		priv->registred = TRUE;
		priv->player_id = holder->player_id;
	}


	g_signal_emit(self, _signals[CONNECTED], 0, ok);

}

GList *mb_net_client_server_get_games(MbNetClientServer * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_mutex_lock(priv->games_lock);
	GList *t = priv->games;
	priv->games = NULL;


	g_list_foreach(t, (GFunc) _copy_holder, self);
	g_mutex_unlock(priv->games_lock);
	return t;
}


static void
mb_net_client_server_get_property(GObject * object, guint prop_id,
				  GValue * value, GParamSpec * param_spec)
{
	MbNetClientServer *self;

	self = MB_NET_CLIENT_SERVER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_client_server_set_property(GObject * object, guint prop_id,
				  const GValue * value,
				  GParamSpec * param_spec)
{
	MbNetClientServer *self;

	self = MB_NET_CLIENT_SERVER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_client_server_class_init(MbNetClientServerClass *
				mb_net_client_server_class)
{
	GObjectClass *g_object_class;

	parent_class =
	    g_type_class_peek_parent(mb_net_client_server_class);


	g_type_class_add_private(mb_net_client_server_class,
				 sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_client_server_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_client_server_set_property;
	g_object_class->get_property = mb_net_client_server_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_client_server_finalize;


	_signals[CONNECTED] =
	    g_signal_new("connected", MB_NET_TYPE_CLIENT_SERVER,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetClientServerClass,
					 connected), NULL, NULL,
			 g_cclosure_marshal_VOID__BOOLEAN, G_TYPE_NONE, 1,
			 G_TYPE_BOOLEAN);

	_signals[NEW_GAME_LIST] =
	    g_signal_new("new_game_list", MB_NET_TYPE_CLIENT_SERVER,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetClientServerClass,
					 new_game_list), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	_signals[GAME_CREATED] =
	    g_signal_new("game_created", MB_NET_TYPE_CLIENT_SERVER,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetClientServerClass,
					 game_created), NULL, NULL,
			 g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
			 G_TYPE_POINTER);

}
