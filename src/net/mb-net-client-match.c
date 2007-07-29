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

#include "mb-net-client-match.h"

#include <glib.h>
#include <glib-object.h>
#include <util/clock.h>

#define INIT_BUBBLES_COUNT 8+7+8+7

typedef struct __ObservedPlayer {
	guint32 player_id;
	gboolean lost;
	Color *bubbles;
	guint32 bubbles_count;
	gboolean odd;
	int score;
} _ObservedPlayer;

typedef struct _Private {
	MbNetMatchHandler *handler;
	MbNetMatchHandler *observer_handler;
	MbNetConnection *con;
	guint32 player_id;
	guint32 match_id;
	guint32 observer_match_id;
	Monkey *monkey;
	MbClock *clock;
	GMutex *lock;
	Bubble **next_row;
	Bubble **waiting_bubbles;
	gboolean bubble_sticked;
	gboolean can_shoot;
	guint32 last_shoot;
	guint32 last_sticked;
	gboolean running;
	GList *observed_players;
	guint32 score;
	MbNetHandlerManager *manager;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	START,
	STOP,
	WINLOST,
	PLAYER_CHANGED,
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_client_match_get_property(GObject * object,
					     guint prop_id,
					     GValue * value,
					     GParamSpec * param_spec);
static void mb_net_client_match_set_property(GObject * object,
					     guint prop_id,
					     const GValue * value,
					     GParamSpec * param_spec);


static guint _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetClientMatch, mb_net_client_match,
			G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_CLIENT_MATCH, Private))




static void mb_net_client_match_finalize(MbNetClientMatch * self);

static void mb_net_client_match_init(MbNetClientMatch * self);

static void _match_init(MbNetMatchHandler * h, MbNetConnection * con,
			guint32 handler_id, MbNetMatchInitStruct * init,
			MbNetClientMatch * self);

static void _penality(MbNetMatchHandler * h, MbNetConnection * con,
		      guint32 handler_id, guint32 time,
		      guint32 penality, MbNetClientMatch * self);
static void _next_row(MbNetMatchHandler * h, MbNetConnection * con,
		      guint32 handler_id, Color * bubbles,
		      MbNetClientMatch * self);


static void _new_cannon_bubble(MbNetMatchHandler * h,
			       MbNetConnection * con,
			       guint32 handler_id, Color color,
			       MbNetClientMatch * self);

static void _penality_bubbles(MbNetMatchHandler * h,
			      MbNetConnection * con,
			      guint32 handler_id, Color * bubbles,
			      MbNetClientMatch * self);
static void _winlost(MbNetMatchHandler * h, MbNetConnection * con,
		     guint32 handler_id, gboolean win,
		     MbNetClientMatch * self);

static void _stop(MbNetMatchHandler * h, MbNetConnection * con,
		  guint32 handler_id, MbNetClientMatch * self);

static void _bubble_sticked(Monkey * monkey, Bubble * bubble,
			    MbNetClientMatch * self);
static void _bubble_shot(Monkey * monkey, Bubble * bubble,
			 MbNetClientMatch * self);

static gint _update_monkey(MbNetClientMatch * self);

static void _start(MbNetMatchHandler * handler, MbNetConnection * con,
		   guint32 handler_id, MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_signal_connect(priv->handler, "penality", (GCallback) _penality,
			 self);
	g_signal_connect(priv->handler, "next-row", (GCallback) _next_row,
			 self);
	g_signal_connect(priv->handler, "new-cannon-bubble",
			 (GCallback) _new_cannon_bubble, self);
	g_signal_connect(priv->handler, "penality-bubbles",
			 (GCallback) _penality_bubbles, self);
	g_signal_connect(priv->handler, "winlost", (GCallback) _winlost,
			 self);

	g_signal_connect(priv->handler, "stop", (GCallback) _stop, self);
	priv->running = TRUE;
	priv->can_shoot = TRUE;
	mb_clock_start(priv->clock);
	gtk_timeout_add(10, (GSourceFunc) _update_monkey, self);

	g_signal_emit(self, _signals[START], 0);

}

static _ObservedPlayer *_get_observed_player(MbNetClientMatch * self,
					     guint32 player_id)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	GList *next = priv->observed_players;
	_ObservedPlayer *p = NULL;
	while (next != NULL) {
		_ObservedPlayer *op = (_ObservedPlayer *) next->data;
		if (op->player_id == player_id) {
			p = op;
		}
		next = g_list_next(next);
	}

	if (p == NULL) {
		p = g_new0(_ObservedPlayer, 1);
		p->player_id = player_id;
		priv->observed_players =
		    g_list_append(priv->observed_players, p);
	}

	return p;

}

static void _observer_player_match(MbNetMatchHandler * handler,
				   guint32 player_id,
				   MbNetMatchInitStruct * match,
				   MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->lock);
	_ObservedPlayer *p = _get_observed_player(self, player_id);
	p->bubbles_count = match->bubbles_count;
	p->bubbles = match->bubbles;
	p->odd = match->odd;
	p->score = match->score;
	int index = g_list_index(priv->observed_players, p);
	g_mutex_unlock(priv->lock);

	g_signal_emit(self, _signals[PLAYER_CHANGED], 0, index);
}

static void _observer_player_winlost(MbNetMatchHandler * h,
				     guint32 player_id, gboolean winlost)
{
}

static void _stop(MbNetMatchHandler * h, MbNetConnection * con,
		  guint32 handler_id, MbNetClientMatch * self)
{
	g_signal_emit(self, _signals[STOP], 0);
}

void mb_net_client_match_ready(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_signal_connect(priv->handler, "match-init",
			 (GCallback) _match_init, self);

	g_signal_connect(priv->observer_handler, "observer-player-match",
			 (GCallback) _observer_player_match, self);

	g_signal_connect(priv->observer_handler, "observer-player-winlost",
			 (GCallback) _observer_player_winlost, self);

	mb_net_match_handler_send_ready(priv->observer_handler, priv->con,
					priv->observer_match_id,
					priv->player_id);

	mb_net_match_handler_send_ready(priv->handler, priv->con,
					priv->match_id, priv->player_id);

}


MbNetClientMatch *mb_net_client_match_new(guint32 match_id,
					  guint32 observer_match_id,
					  guint32 player_id,
					  MbNetConnection * con,
					  MbNetHandlerManager * manager)
{

	MbNetClientMatch *self;

	self =
	    MB_NET_CLIENT_MATCH(g_object_new
				(MB_NET_TYPE_CLIENT_MATCH, NULL));

	Private *priv;
	priv = GET_PRIVATE(self);

	priv->match_id = match_id;
	priv->observer_match_id = observer_match_id;
	priv->player_id = player_id;
	priv->con = con;
	g_object_ref(priv->con);
	priv->monkey = monkey_new(TRUE);
	priv->manager = manager;
	g_signal_connect(priv->monkey, "bubble-sticked",
			 (GCallback) _bubble_sticked, self);
	g_signal_connect(priv->monkey, "bubble-shot",
			 (GCallback) _bubble_shot, self);


	mb_net_handler_manager_register(manager,
					MB_NET_HANDLER(priv->handler));
	mb_net_handler_manager_register(manager,
					MB_NET_HANDLER(priv->
						       observer_handler));

	return self;
}

static void mb_net_client_match_init(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));
	priv->observer_handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));

	g_signal_connect(priv->handler, "start", (GCallback) _start, self);
	priv->lock = g_mutex_new();
	priv->clock = mb_clock_new();

}

static void mb_net_client_match_finalize(MbNetClientMatch * self)
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
	priv->handler = NULL;

	g_object_unref(priv->observer_handler);
	priv->observer_handler = NULL;

	g_object_unref(priv->monkey);
	g_object_unref(priv->clock);
	g_mutex_free(priv->lock);
	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}
void mb_net_client_match_lock(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_mutex_lock(priv->lock);
}

void mb_net_client_match_unlock(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_mutex_unlock(priv->lock);
}

Monkey *mb_net_client_match_get_monkey(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	return priv->monkey;
}

guint32 mb_net_client_match_get_id(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	return priv->match_id;
}

int mb_net_client_match_get_score(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	return priv->score;
}

guint32 mb_net_client_match_get_time(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	return mb_clock_get_time(priv->clock);
}
static void _match_init(MbNetMatchHandler * h, MbNetConnection * con,
			guint32 handler_id, MbNetMatchInitStruct * init,
			MbNetClientMatch * self)
{

	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->lock);

	Monkey *m = priv->monkey;
	Shooter *s = monkey_get_shooter(m);
	Bubble **bubbles =
	    (Bubble **) g_new0(Bubble *, INIT_BUBBLES_COUNT);
	int i;
	for (i = 0; i < INIT_BUBBLES_COUNT; i++) {
		bubbles[i] = bubble_new(init->bubbles[i], 0, 0);
	}
	board_init(playground_get_board(monkey_get_playground(m)),
		   bubbles, INIT_BUBBLES_COUNT);

	shooter_add_bubble(s, bubble_new(init->bubble1, 0, 0));
	shooter_add_bubble(s, bubble_new(init->bubble2, 0, 0));
	priv->score = init->score;
	g_mutex_unlock(priv->lock);
}

static void _penality(MbNetMatchHandler * h, MbNetConnection * con,
		      guint32 handler_id, guint32 time,
		      guint32 penality, MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->lock);
	Monkey *m = priv->monkey;
	monkey_add_bubbles(m, penality);
	g_mutex_unlock(priv->lock);
}

static void _next_row(MbNetMatchHandler * h, MbNetConnection * con,
		      guint32 handler_id, Color * colors,
		      MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->lock);

	Bubble **bubbles = g_new(Bubble *, 8);
	int i;
	for (i = 0; i < 8; i++) {
		if (colors[i] != NO_COLOR) {
			bubbles[i] = bubble_new(colors[i], 0, 0);
		}
	}


	priv->next_row = bubbles;

	g_mutex_unlock(priv->lock);
}



static void _new_cannon_bubble(MbNetMatchHandler * h,
			       MbNetConnection * con,
			       guint32 handler_id, Color color,
			       MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->lock);
	Monkey *m = priv->monkey;
	shooter_add_bubble(monkey_get_shooter(m), bubble_new(color, 0, 0));
	g_mutex_unlock(priv->lock);
}


static void _add_waiting_bubbles(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);


	Monkey *m = priv->monkey;

	monkey_add_waiting_row_complete(m, priv->waiting_bubbles);
	priv->waiting_bubbles = NULL;
	priv->can_shoot = TRUE;

}

static void _penality_bubbles(MbNetMatchHandler * h,
			      MbNetConnection * con,
			      guint32 handler_id, Color * colors,
			      MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->lock);

	Bubble **bubbles = g_new0(Bubble *, 7);
	int i;
	for (i = 0; i < 7; i++) {
		if (colors[i] != NO_COLOR) {
			bubbles[i] = bubble_new(colors[i], 0, 0);
		}
	}
	priv->waiting_bubbles = bubbles;

	if (priv->bubble_sticked == TRUE) {
		_add_waiting_bubbles(self);
	}
	g_mutex_unlock(priv->lock);

}

static void _winlost(MbNetMatchHandler * h, MbNetConnection * con,
		     guint32 handler_id, gboolean win,
		     MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->running = FALSE;
	g_signal_emit(self, _signals[WINLOST], 0, win);
}

static void _bubble_shot(Monkey * monkey, Bubble * bubble,
			 MbNetClientMatch * self)
{

	Private *priv;
	priv = GET_PRIVATE(self);
	Monkey *m = priv->monkey;

	priv->last_shoot = mb_clock_get_time(priv->clock);
	priv->bubble_sticked = FALSE;
	if (monkey_get_waiting_bubbles_count(m) > 0) {
		priv->can_shoot = FALSE;


	}

	Shooter *s = monkey_get_shooter(m);
	mb_net_match_handler_send_shoot(priv->handler, priv->con,
					priv->match_id, priv->last_shoot,
					shooter_get_angle(s));

}

static void _bubble_sticked(Monkey * monkey, Bubble * bubble,
			    MbNetClientMatch * self)
{


	Private *priv;
	priv = GET_PRIVATE(self);
	Monkey *m = priv->monkey;

	priv->last_sticked = mb_clock_get_time(priv->clock);
	priv->bubble_sticked = TRUE;
	priv->can_shoot = TRUE;
	if (priv->waiting_bubbles != NULL) {
		_add_waiting_bubbles(self);
	}

	if ((monkey_get_shot_count(m) % 8) == 0) {
		monkey_insert_bubbles(m, priv->next_row);

	}


}
void mb_net_client_match_shoot(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_mutex_lock(priv->lock);
	Monkey *m = priv->monkey;
	if (priv->can_shoot == TRUE) {
		monkey_shoot(m, mb_clock_get_time(priv->clock));
	}

	g_mutex_unlock(priv->lock);
}


static gint _update_monkey(MbNetClientMatch * self)
{


	Private *priv;
	priv = GET_PRIVATE(self);
	//g_print("update ... \n");
	g_mutex_lock(priv->lock);
	Monkey *m = priv->monkey;
	//g_print("update2 ... \n");
	gint time;
	time = mb_clock_get_time(priv->clock);

	if (priv->running == TRUE) {

		if ((time - priv->last_sticked) > 10000) {
			if (priv->can_shoot == TRUE) {
				monkey_shoot(m, time);
			}
		}
		monkey_update(m, time);

	}
	g_mutex_unlock(priv->lock);

	return priv->running;
}



void mb_net_client_match_get_player_bubbles(MbNetClientMatch * self,
					    int player, Color ** color,
					    int *count, gboolean * odd)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_mutex_lock(priv->lock);
	_ObservedPlayer *p =
	    (_ObservedPlayer *) g_list_nth(priv->observed_players,
					   player)->data;
	*color = p->bubbles;
	*count = p->bubbles_count;
	*odd = p->odd;
	g_mutex_unlock(priv->lock);
	return;
}


int mb_net_client_match_get_player_score(MbNetClientMatch * self,
					 int player)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_mutex_lock(priv->lock);
	_ObservedPlayer *p =
	    (_ObservedPlayer *) g_list_nth(priv->observed_players,
					   player)->data;
	int score = p->score;
	g_mutex_unlock(priv->lock);
	return score;
}

gboolean mb_net_client_match_is_player_lost(MbNetClientMatch * self,
					    int player)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_mutex_lock(priv->lock);
	_ObservedPlayer *p =
	    (_ObservedPlayer *) g_list_nth(priv->observed_players,
					   player)->data;

	g_mutex_unlock(priv->lock);
	return p->lost;
}



static void
mb_net_client_match_get_property(GObject * object, guint prop_id,
				 GValue * value, GParamSpec * param_spec)
{
	MbNetClientMatch *self;

	self = MB_NET_CLIENT_MATCH(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_client_match_set_property(GObject * object, guint prop_id,
				 const GValue * value,
				 GParamSpec * param_spec)
{
	MbNetClientMatch *self;

	self = MB_NET_CLIENT_MATCH(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_client_match_class_init(MbNetClientMatchClass *
			       mb_net_client_match_class)
{
	GObjectClass *g_object_class;

	parent_class = g_type_class_peek_parent(mb_net_client_match_class);


	g_type_class_add_private(mb_net_client_match_class,
				 sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_client_match_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_client_match_set_property;
	g_object_class->get_property = mb_net_client_match_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_client_match_finalize;



	_signals[START] =
	    g_signal_new("start", MB_NET_TYPE_CLIENT_MATCH,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetClientMatchClass,
					 start), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0,
			 NULL);

	_signals[STOP] =
	    g_signal_new("stop", MB_NET_TYPE_CLIENT_MATCH,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetClientMatchClass,
					 stop), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0,
			 NULL);
	_signals[WINLOST] =
	    g_signal_new("winlost", MB_NET_TYPE_CLIENT_MATCH,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetClientMatchClass, winlost),
			 NULL, NULL, g_cclosure_marshal_VOID__UINT,
			 G_TYPE_NONE, 1, G_TYPE_UINT);

	_signals[PLAYER_CHANGED] =
	    g_signal_new("player-changed", MB_NET_TYPE_CLIENT_MATCH,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetClientMatchClass,
					 player_changed), NULL, NULL,
			 g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1,
			 G_TYPE_UINT);

}
