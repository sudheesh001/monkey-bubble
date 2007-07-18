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
typedef struct _Private {
	MbNetMatchHandler *handler;
	MbNetConnection *con;
	guint32 player_id;
	guint32 match_id;
	Monkey *monkey;
	MbClock *clock;
	GMutex *lock;
	Bubble **next_row;
	Bubble **waiting_bubbles;
	gboolean bubble_sticked;
	gboolean can_shoot;
	guint32 last_shoot;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	START,
	STOP,
	WINLOST,
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
			guint32 handler_id, guint32 count, Color * bubbles,
			gboolean odd, Color bubble1, Color bubble2,
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


static void _bubble_sticked(Monkey * monkey, Bubble * bubble,
			    MbNetClientMatch * self);
static void _bubble_shot(Monkey * monkey, Bubble * bubble,
			 MbNetClientMatch * self);

static void _start(MbNetMatchHandler * handler, MbNetConnection * con,
		   guint32 handler_id, MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_signal_connect(priv->handler, "penality", (GCallback) _penality,
			 self);
	g_signal_connect(priv->handler, "new-row", (GCallback) _next_row,
			 self);
	g_signal_connect(priv->handler, "new-cannon-bubble",
			 (GCallback) _new_cannon_bubble, self);
	g_signal_connect(priv->handler, "penality-bubbles",
			 (GCallback) _penality_bubbles, self);
	g_signal_connect(priv->handler, "winlost", (GCallback) _winlost,
			 self);

	g_signal_emit(self, _signals[START], 0);

}

void mb_net_client_match_ready(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_signal_connect(priv->handler, "match-init",
			 (GCallback) _match_init, self);

	mb_net_match_handler_send_ready(priv->handler, priv->con,
					priv->match_id, priv->player_id);

}


MbNetClientMatch *mb_net_client_match_new(guint32 match_id,
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
	priv->player_id = player_id;
	priv->con = con;

	priv->monkey = monkey_new(TRUE);

	g_signal_connect(priv->monkey, "bubble-sticked",
			 (GCallback) _bubble_sticked, self);
//      g_signal_connect (priv->monkey,"game-lost",(GCallback)_game_list,self);
	g_signal_connect(priv->monkey, "bubble-shot",
			 (GCallback) _bubble_shot, self);


	mb_net_handler_manager_register(manager,
					MB_NET_HANDLER(priv->handler));
	return self;
}

static void mb_net_client_match_init(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));

	g_signal_connect(priv->handler, "start", (GCallback) _start, self);
	priv->lock = g_mutex_new();

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

static void _match_init(MbNetMatchHandler * h, MbNetConnection * con,
			guint32 handler_id, guint32 count, Color * colors,
			gboolean odd, Color bubble1, Color bubble2,
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
		bubbles[i] = bubble_new(colors[i], 0, 0);
	}
	board_init(playground_get_board(monkey_get_playground(m)),
		   bubbles, INIT_BUBBLES_COUNT);

	shooter_add_bubble(s, bubble_new(bubble1, 0, 0));
	shooter_add_bubble(s, bubble_new(bubble2, 0, 0));

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

	g_mutex_lock(priv->lock);

	Monkey *m = priv->monkey;

	monkey_add_waiting_row_complete(m, priv->waiting_bubbles);
	priv->waiting_bubbles = NULL;
	priv->can_shoot = TRUE;
	g_mutex_unlock(priv->lock);

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

	g_mutex_unlock(priv->lock);

	if (priv->bubble_sticked == TRUE) {
		_add_waiting_bubbles(self);
	}

}

static void _winlost(MbNetMatchHandler * h, MbNetConnection * con,
		     guint32 handler_id, gboolean win,
		     MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->lock);

	g_signal_emit(self, _signals[WINLOST], 0, win);
	g_mutex_unlock(priv->lock);
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

	priv->bubble_sticked = TRUE;
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



static void mb_net_client_match_finalize(MbNetClientMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
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
			 NULL, NULL, g_cclosure_marshal_VOID__BOOLEAN,
			 G_TYPE_NONE, 1, G_TYPE_UINT);

}
