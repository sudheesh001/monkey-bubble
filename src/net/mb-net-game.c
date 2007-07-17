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

#include "mb-net-game.h"
#include <net/mb-net-game-handler.h>
#include <glib.h>
#include <glib-object.h>

typedef struct __Player {
	MbNetConnection *con;
	guint32 handler_id;
	gchar *name;
	guint32 score;
} _Player;

typedef struct __Observer {
	MbNetConnection *con;
	guint32 handler_id;
} _Observer;

typedef struct _Private {

	MbNetGameHandler *handler;
	MbNetServerPlayer *master_player;
	GList *players;
	GList *observers;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	STOPPED,
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_game_get_property(GObject * object,
				     guint prop_id,
				     GValue * value,
				     GParamSpec * param_spec);
static void mb_net_game_set_property(GObject * object,
				     guint prop_id,
				     const GValue * value,
				     GParamSpec * param_spec);


static guint _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetGame, mb_net_game, G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_GAME, Private))


static void _stop_game(MbNetGame * self);

static void mb_net_game_finalize(MbNetGame * self);

static void mb_net_game_init(MbNetGame * self);

static void _join(MbNetGame * self, MbNetConnection * con,
		  guint32 handler_id, gboolean observer,
		  MbNetGameHandler * h);
static void _ask_player_list(MbNetGame * self, MbNetConnection * con,
			     guint32 handler_id, MbNetGameHandler * h);
static void _start(MbNetGame * self, MbNetConnection * con,
		   guint32 handler_id, MbNetGameHandler * h);
static void _stop(MbNetGame * self, MbNetConnection * con,
		  guint32 handler_id, MbNetGameHandler * h);
static void _ask_score(MbNetGame * self, MbNetConnection * con,
		       guint32 handler_id, MbNetGameHandler * h);

static void _send_player_list(MbNetGame * self, MbNetConnection * con,
			      guint32 handler_id);
static MbNetScoreHolder *_create_score(MbNetGame * self);
static void _notify_new_player(MbNetGame * self);
static void mb_net_game_init(MbNetGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->handler =
	    MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));

	g_signal_connect_swapped(priv->handler, "join",
				 (GCallback) _join, self);

	g_signal_connect_swapped(priv->handler, "ask-player-list",
				 (GCallback) _ask_player_list, self);

	g_signal_connect_swapped(priv->handler, "start",
				 (GCallback) _start, self);

	g_signal_connect_swapped(priv->handler, "stop",
				 (GCallback) _stop, self);

	g_signal_connect_swapped(priv->handler, "ask-score",
				 (GCallback) _ask_score, self);


}


static void mb_net_game_finalize(MbNetGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_object_unref(priv->handler);
	if (self->info.name != NULL) {
		g_free(self->info.name);
	}
	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}

void _player_disconnected(MbNetGame * self, MbNetServerPlayer * p)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	if (priv->master_player == p) {
		_stop_game(self);
	}

}

MbNetGame *mb_net_game_new(const gchar * name, MbNetServerPlayer * p,
			   MbNetHandlerManager * manager)
{
	Private *priv;
	MbNetGame *self =
	    MB_NET_GAME(g_object_new(MB_NET_TYPE_GAME, NULL));

	priv = GET_PRIVATE(self);
	g_object_ref(p);
	priv->master_player = p;

	g_signal_connect_swapped(p, "disconnected",
				 (GCallback) _player_disconnected, self);


	mb_net_handler_manager_register(manager,
					MB_NET_HANDLER(priv->handler));

	self->info.name = g_strdup(name);
	self->info.handler_id =
	    mb_net_handler_get_id(MB_NET_HANDLER(priv->handler));

	return self;
}

static MbNetScoreHolder *_create_score(MbNetGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	MbNetScoreHolder *h;
	h = (MbNetScoreHolder *) g_new0(MbNetScoreHolder, 1);
	GList *next = priv->players;
	while (next != NULL) {
		MbNetPlayerScoreHolder *ph;
		_Player *p = (_Player *) next->data;
		ph = (MbNetPlayerScoreHolder *)
		    g_new0(MbNetPlayerScoreHolder, 1);
		ph->name = g_strdup(p->name);
		ph->score = p->score;
		h->score_by_player = g_list_append(h->score_by_player, ph);
		next = g_list_next(next);
	}

	return h;

}
static MbNetPlayerListHolder *_create_player_list(MbNetGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	MbNetPlayerListHolder *h;
	h = (MbNetPlayerListHolder *) g_new0(MbNetPlayerListHolder, 1);
	GList *next = priv->players;
	while (next != NULL) {
		MbNetPlayerHolder *ph;
		_Player *p = (_Player *) next->data;
		ph = (MbNetPlayerHolder *) g_new0(MbNetPlayerHolder, 1);
		ph->name = g_strdup(p->name);
		h->players = g_list_append(h->players, ph);
		next = g_list_next(next);
	}

	return h;

}
static void _send_player_list(MbNetGame * self, MbNetConnection * con,
			      guint32 handler_id)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	MbNetPlayerListHolder *h = _create_player_list(self);
	mb_net_game_handler_send_player_list(priv->handler, con,
					     handler_id, h);
	mb_net_player_list_holder_free(h);
}

static void _join(MbNetGame * self, MbNetConnection * con,
		  guint32 handler_id, gboolean observer,
		  MbNetGameHandler * h)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	if (observer == TRUE) {
		_Observer *o = (_Observer *) g_new0(_Observer, 1);
		o->con = con;
		g_object_ref(con);
		o->handler_id = handler_id;
		priv->observers = g_list_append(priv->observers, o);
		mb_net_game_handler_send_join_response(h, con, handler_id,
						       TRUE);
	} else {
		_Player *p = (_Player *) g_new0(_Player, 1);
		p->con = con;
		g_object_ref(con);
		p->handler_id = handler_id;
		p->name = g_strdup("toto");
		p->score = 0;
		priv->players = g_list_append(priv->players, p);
		mb_net_game_handler_send_join_response(h, con, handler_id,
						       TRUE);

		_notify_new_player(self);
	}

}

static void _notify_new_player(MbNetGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	MbNetPlayerListHolder *h = _create_player_list(self);
	GList *next = priv->players;
	while (next != NULL) {
		_Player *p = (_Player *) next->data;
		mb_net_game_handler_send_player_list(priv->handler, p->con,
						     p->handler_id, h);
		next = g_list_next(next);
	}
	next = priv->observers;
	while (next != NULL) {
		_Observer *o = (_Observer *) next->data;
		mb_net_game_handler_send_player_list(priv->handler, o->con,
						     o->handler_id, h);
		next = g_list_next(next);
	}

}
static void _ask_player_list(MbNetGame * self, MbNetConnection * con,
			     guint32 handler_id, MbNetGameHandler * h)
{
	_send_player_list(self, con, handler_id);
}

static void _start(MbNetGame * self, MbNetConnection * con,
		   guint32 handler_id, MbNetGameHandler * h)
{
}

static void _stop(MbNetGame * self, MbNetConnection * con,
		  guint32 handler_id, MbNetGameHandler * h)
{
	_stop_game(self);
}


static void _stop_game(MbNetGame * self)
{
	g_signal_emit(self, _signals[STOPPED], 0);
}

static void _ask_score(MbNetGame * self, MbNetConnection * con,
		       guint32 handler_id, MbNetGameHandler * h)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	MbNetScoreHolder *holder = _create_score(self);
	mb_net_game_handler_send_score(priv->handler, con, handler_id,
				       holder);
	mb_net_score_holder_free(holder);
}



MbNetHandler *mb_net_game_get_handler(MbNetGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	return MB_NET_HANDLER(priv->handler);
}

static void
mb_net_game_get_property(GObject * object, guint prop_id, GValue * value,
			 GParamSpec * param_spec)
{
	MbNetGame *self;

	self = MB_NET_GAME(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_game_set_property(GObject * object, guint prop_id,
			 const GValue * value, GParamSpec * param_spec)
{
	MbNetGame *self;

	self = MB_NET_GAME(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}


static void mb_net_game_class_init(MbNetGameClass * mb_net_game_class)
{
	GObjectClass *g_object_class;

	parent_class = g_type_class_peek_parent(mb_net_game_class);


	g_type_class_add_private(mb_net_game_class, sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_game_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_game_set_property;
	g_object_class->get_property = mb_net_game_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_game_finalize;

	_signals[STOPPED] =
	    g_signal_new("stopped", MB_NET_TYPE_GAME,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetGameClass,
					 stopped), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0,
			 NULL);

}
