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

#include "mb-net-client-game.h"
#include <net/mb-net-game-handler.h>
#include <glib.h>
#include <glib-object.h>


typedef struct _Private {
	MbNetGameHandler *handler;
	MbNetGameHandler *observer_handler;
	MbNetHandlerManager *manager;
	guint32 player_id;
	guint32 game_id;
	guint32 observed_match_id;
	MbNetConnection *con;
	GList *players;
	GMutex *players_mutex;
	GList *scores;
	gboolean master;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	JOIN_RESPONSE,
	PLAYER_LIST_CHANGED,
	SCORE_CHANGED,
	START,
	STOP,
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_client_game_get_property(GObject * object,
					    guint prop_id,
					    GValue * value,
					    GParamSpec * param_spec);
static void mb_net_client_game_set_property(GObject * object,
					    guint prop_id,
					    const GValue * value,
					    GParamSpec * param_spec);


static guint _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetClientGame, mb_net_client_game, G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_CLIENT_GAME, Private))



static void _join_response(MbNetGameHandler * h, MbNetConnection * c,
			   guint32 handler_id, gboolean ok,
			   gboolean master, MbNetClientGame * self);
static void _player_list(MbNetGameHandler * h, MbNetConnection * con,
			 guint32 handler_id,
			 MbNetPlayerListHolder * holder,
			 MbNetClientGame * self);
static void _observer_match_created(MbNetGameHandler * handler,
				    MbNetConnection * con,
				    guint32 handler_id, guint32 match_id,
				    MbNetClientGame * self);
static void _match_created(MbNetGameHandler * handler,
			   MbNetConnection * con, guint32 handler_id,
			   guint32 match_id, MbNetClientGame * self);
static void _stop(MbNetGameHandler * handler, MbNetConnection * con,
		  guint32 handler_id, MbNetClientGame * self);
static void _score(MbNetGameHandler * h, MbNetConnection * con,
		   guint32 handler_id, MbNetScoreHolder * holder,
		   MbNetClientGame * self);


static void mb_net_client_game_finalize(MbNetClientGame * self);

static void mb_net_client_game_init(MbNetClientGame * self);


MbNetClientGame *mb_net_client_game_create(guint32 id, guint32 player_id,
					   MbNetConnection * con,
					   MbNetHandlerManager * manager)
{
	MbNetClientGame *self;

	self =
	    MB_NET_CLIENT_GAME(g_object_new
			       (MB_NET_TYPE_CLIENT_GAME, NULL));

	Private *priv;
	priv = GET_PRIVATE(self);

	priv->handler =
	    MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));

	priv->observer_handler =
	    MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));

	g_signal_connect(priv->handler, "join-response",
			 (GCallback) _join_response, self);

	g_signal_connect(priv->handler, "player-list",
			 (GCallback) _player_list, self);

	g_signal_connect(priv->handler, "score", (GCallback) _score, self);

	g_signal_connect(priv->handler, "match-created",
			 (GCallback) _match_created, self);

	g_signal_connect(priv->observer_handler, "match-created",
			 (GCallback) _observer_match_created, self);
	g_signal_connect(priv->handler, "stop", (GCallback) _stop, self);

	mb_net_handler_manager_register(manager,
					MB_NET_HANDLER(priv->handler));

	mb_net_handler_manager_register(manager,
					MB_NET_HANDLER(priv->
						       observer_handler));

	g_object_ref(manager);
	priv->manager = manager;
	priv->game_id = id;
	priv->player_id = player_id;
	g_object_ref(con);
	priv->con = con;
	return self;
}


static void mb_net_client_game_init(MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->players_mutex = g_mutex_new();
}

static void mb_net_client_game_finalize(MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	mb_net_handler_manager_unregister(priv->manager,
					  mb_net_handler_get_id
					  (MB_NET_HANDLER(priv->handler)));
	mb_net_handler_manager_unregister(priv->manager,
					  mb_net_handler_get_id
					  (MB_NET_HANDLER
					   (priv->observer_handler)));
	g_object_unref(priv->handler);
	g_object_unref(priv->observer_handler);
	g_object_unref(priv->manager);
	g_object_unref(priv->con);
	g_mutex_free(priv->players_mutex);
	g_list_foreach(priv->players, (GFunc) mb_net_player_holder_free,
		       NULL);
	g_list_free(priv->players);

	g_list_foreach(priv->scores,
		       (GFunc) mb_net_player_score_holder_free, NULL);
	g_list_free(priv->scores);
	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}


static void _join_response(MbNetGameHandler * h, MbNetConnection * c,
			   guint32 handler_id, gboolean ok,
			   gboolean master, MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->master = master;
	g_signal_emit(self, _signals[JOIN_RESPONSE], 0, ok);

}

guint32 mb_net_client_game_get_game_id(MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	return priv->game_id;
}

static void _copy_holder(MbNetPlayerHolder * h, MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	priv->players =
	    g_list_append(priv->players,
			  mb_net_player_holder_create(h->name));
}

static void _player_list(MbNetGameHandler * h, MbNetConnection * con,
			 guint32 handler_id,
			 MbNetPlayerListHolder * holder,
			 MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->players_mutex);


	g_list_foreach(priv->players, (GFunc) mb_net_player_holder_free,
		       NULL);
	g_list_free(priv->players);
	priv->players = NULL;


	g_list_foreach(holder->players, (GFunc) _copy_holder, self);

	g_mutex_unlock(priv->players_mutex);
	g_signal_emit(self, _signals[PLAYER_LIST_CHANGED], 0);
}

static void _observer_match_created(MbNetGameHandler * handler,
				    MbNetConnection * con,
				    guint32 handler_id, guint32 match_id,
				    MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	priv->observed_match_id = match_id;
}

static void _match_created(MbNetGameHandler * handler,
			   MbNetConnection * con, guint32 handler_id,
			   guint32 match_id, MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	MbNetClientMatch *match =
	    mb_net_client_match_new(match_id, priv->observed_match_id,
				    priv->player_id, priv->con,
				    priv->manager);
	g_signal_emit(self, _signals[START], 0, match);
}

static void _stop(MbNetGameHandler * handler, MbNetConnection * con,
		  guint32 handler_id, MbNetClientGame * self)
{
	g_print("stopped \n");
	g_signal_emit(self, _signals[STOP], 0);
}

void mb_net_client_game_ask_player_list(MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	mb_net_game_handler_send_ask_player_list(priv->handler, priv->con,
						 priv->game_id);
}

static void _copy_score_holder(MbNetPlayerScoreHolder * h,
			       MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	priv->scores =
	    g_list_append(priv->scores,
			  mb_net_player_score_holder_create(h->name,
							    h->score));
}
static void _score(MbNetGameHandler * h, MbNetConnection * con,
		   guint32 handler_id, MbNetScoreHolder * holder,
		   MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->players_mutex);


	g_list_foreach(priv->scores,
		       (GFunc) mb_net_player_score_holder_free, NULL);
	g_list_free(priv->scores);
	priv->scores = NULL;


	g_list_foreach(holder->score_by_player, (GFunc) _copy_score_holder,
		       self);

	g_mutex_unlock(priv->players_mutex);
	g_signal_emit(self, _signals[SCORE_CHANGED], 0);
}


void mb_net_client_game_ask_score(MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	mb_net_game_handler_send_ask_score(priv->handler, priv->con,
					   priv->game_id);
}


GList *mb_net_client_game_get_players(MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->players_mutex);
	GList *t = priv->players;
	priv->players = NULL;


	g_list_foreach(t, (GFunc) _copy_holder, self);
	g_mutex_unlock(priv->players_mutex);
	return t;

}

void mb_net_client_game_start(MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	mb_net_game_handler_send_start(priv->handler, priv->con,
				       priv->game_id);
}

void mb_net_client_game_stop(MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	mb_net_game_handler_send_stop(priv->handler, priv->con,
				      priv->game_id);
}


void mb_net_client_game_join(MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	mb_net_game_handler_send_join(priv->handler, priv->con,
				      priv->game_id, priv->player_id,
				      FALSE);
	mb_net_game_handler_send_join(priv->observer_handler, priv->con,
				      priv->game_id, priv->player_id,
				      TRUE);

}

gboolean mb_net_client_game_is_master(MbNetClientGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	return priv->master;
}


static void
mb_net_client_game_get_property(GObject * object, guint prop_id,
				GValue * value, GParamSpec * param_spec)
{
	MbNetClientGame *self;

	self = MB_NET_CLIENT_GAME(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_client_game_set_property(GObject * object, guint prop_id,
				const GValue * value,
				GParamSpec * param_spec)
{
	MbNetClientGame *self;

	self = MB_NET_CLIENT_GAME(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_client_game_class_init(MbNetClientGameClass *
			      mb_net_client_game_class)
{
	GObjectClass *g_object_class;

	parent_class = g_type_class_peek_parent(mb_net_client_game_class);


	g_type_class_add_private(mb_net_client_game_class,
				 sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_client_game_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_client_game_set_property;
	g_object_class->get_property = mb_net_client_game_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_client_game_finalize;

	_signals[JOIN_RESPONSE] =
	    g_signal_new("join-response", MB_NET_TYPE_CLIENT_GAME,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetClientGameClass,
					 join_response), NULL, NULL,
			 g_cclosure_marshal_VOID__BOOLEAN, G_TYPE_NONE, 1,
			 G_TYPE_BOOLEAN);

	_signals[PLAYER_LIST_CHANGED] =
	    g_signal_new("player-list-changed", MB_NET_TYPE_CLIENT_GAME,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetClientGameClass,
					 player_list_changed), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0,
			 NULL);

	_signals[SCORE_CHANGED] =
	    g_signal_new("score-changed", MB_NET_TYPE_CLIENT_GAME,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetClientGameClass,
					 score_changed), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0,
			 NULL);

	_signals[START] =
	    g_signal_new("start", MB_NET_TYPE_CLIENT_GAME,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetClientGameClass,
					 start), NULL, NULL,
			 g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
			 G_TYPE_POINTER);
	_signals[STOP] =
	    g_signal_new("stop", MB_NET_TYPE_CLIENT_GAME,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetClientGameClass,
					 stop), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0,
			 NULL);
}
