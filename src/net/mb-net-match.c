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

#include "mb-net-match.h"
#include <net/mb-net-match-handler.h>
#include <glib.h>
#include <glib-object.h>
#include <monkey/monkey.h>
#include <util/clock.h>
#define INIT_BUBBLES_COUNT 8+7+8+7

typedef struct __Player {
	MbNetConnection *con;
	MbNetServerPlayer *player;
	guint32 score;
	guint32 handler_id;
	gboolean lost;
	Monkey *monkey;
	Color *waiting_bubbles;
} _Player;


typedef struct _Private {
	MbNetHandlerManager *manager;
	MbNetMatchHandler *handler;
	GList *waited_players;
	GList *players;
	GList *observers;
	MbNetServerPlayer *master;
	GMutex *players_mutex;

	gboolean running;
	MbClock *clock;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_match_get_property(GObject * object,
				      guint prop_id,
				      GValue * value,
				      GParamSpec * param_spec);
static void mb_net_match_set_property(GObject * object,
				      guint prop_id,
				      const GValue * value,
				      GParamSpec * param_spec);


//static        guint   _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetMatch, mb_net_match, G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_MATCH, Private))



static void _start_match(MbNetMatch * self);

static void _remove_player(MbNetMatch * self, MbNetServerPlayer * p);

static void mb_net_match_finalize(MbNetMatch * self);

static void mb_net_match_init(MbNetMatch * self);



static void mb_net_match_init(MbNetMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	priv->handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));

	priv->players_mutex = g_mutex_new();
	priv->clock = mb_clock_new();
}


static void mb_net_match_finalize(MbNetMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}

static void _disconnected(MbNetServerPlayer * p, MbNetMatch * self)
{
	_remove_player(self, p);
}

static _Player *_get_player_by_handler_id(MbNetMatch * self,
					  guint32 handler_id)
{

	Private *priv;
	priv = GET_PRIVATE(self);
	g_mutex_lock(priv->players_mutex);
	GList *next = priv->players;
	_Player *ret = NULL;
	while (next != NULL) {
		_Player *p;
		p = (_Player *) next->data;
		if (p->handler_id == handler_id) {
			ret = p;
			break;
		}

		next = g_list_next(next);
	}
	g_mutex_unlock(priv->players_mutex);
	return ret;
}
static MbNetServerPlayer *_get_waited_player(MbNetMatch * self,
					     guint32 player_id)
{

	Private *priv;
	priv = GET_PRIVATE(self);
	g_mutex_lock(priv->players_mutex);
	GList *next = priv->waited_players;
	MbNetServerPlayer *ret = NULL;
	while (next != NULL) {
		MbNetServerPlayer *p;
		p = MB_NET_SERVER_PLAYER(next->data);
		if (mb_net_server_player_get_id(p) == player_id) {
			ret = p;
			break;
		}

		next = g_list_next(next);
	}
	g_mutex_unlock(priv->players_mutex);
	return ret;
}

static void _ready(MbNetMatchHandler * handler, MbNetConnection * con,
		   guint32 handler_id, guint32 player_id,
		   MbNetMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	if (priv->running == FALSE) {
		MbNetServerPlayer *p = _get_waited_player(self, player_id);
		if (p == NULL)
			return;

		g_mutex_lock(priv->players_mutex);
		priv->waited_players =
		    g_list_remove(priv->waited_players, p);
		_Player *player = (_Player *) g_new0(_Player, 1);
		player->player = p;
		player->handler_id = handler_id;
		player->con = con;

		priv->players = g_list_append(priv->players, player);

		gboolean start = FALSE;
		start = g_list_length(priv->waited_players) == 0;
		g_mutex_unlock(priv->players_mutex);

		if (start) {
			_start_match(self);
		}
	}

}
MbNetMatch *mb_net_match_new(MbNetServerPlayer * master, GList * players,
			     GList * observers,
			     MbNetHandlerManager * manager)
{
	Private *priv;
	MbNetMatch *self =
	    MB_NET_MATCH(g_object_new(MB_NET_TYPE_MATCH, NULL));

	priv = GET_PRIVATE(self);
	priv->manager = manager;
	g_object_ref(master);
	priv->master = master;

	priv->manager = manager;
	mb_net_handler_manager_register(manager,
					MB_NET_HANDLER(priv->handler));

	g_mutex_lock(priv->players_mutex);

	GList *next = players;
	while (next != NULL) {
		MbNetServerPlayer *p;
		p = MB_NET_SERVER_PLAYER(next->data);
		g_object_ref(p);
		g_signal_connect(p, "disconnected",
				 (GCallback) _disconnected, self);
		priv->waited_players =
		    g_list_append(priv->waited_players, p);
		next = g_list_next(next);
	}
	g_mutex_unlock(priv->players_mutex);

	g_signal_connect(priv->handler, "ready", (GCallback) _ready, self);
	return self;
}


guint32 mb_net_match_get_id(MbNetMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	return mb_net_handler_get_id(MB_NET_HANDLER(priv->handler));
}


static void _bubble_sticked(Monkey * monkey, Bubble * b, _Player * c)
{
//      g_print("bubble sticked on server .. \n");
}

static void
_bubbles_exploded(Monkey * monkey,
		  GList * exploded, GList * fallen, _Player * c)
{

}


static void _game_lost(Monkey * m, _Player * c)
{

}

static void _shoot(MbNetMatchHandler * handler, MbNetConnection * con,
		   guint32 handler_id, guint32 time, gfloat radian,
		   MbNetMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	_Player *p = _get_player_by_handler_id(self, handler_id);
	if (p == NULL) {
		g_print("unknow player \n");
		return;
	}
	g_mutex_lock(priv->players_mutex);

	if (monkey_get_waiting_bubbles_count(p->monkey) > 0) {
		int i;

		Color *colors = g_new0(Color, 7);
		for (i = 0; i < 7; i++) {
			colors[i] = NO_COLOR;
		}

		int count =
		    MIN(monkey_get_waiting_bubbles_count(p->monkey), 7);
		int empty = 7;
		int j;

		for (j = 0; j < count; j++) {
			int c = rand() % empty;
			int i;
			for (i = 0; i < 7; i++) {
				if ((c <= 0) && (colors[i] == NO_COLOR)) {

					colors[i] = rand() % COLORS_COUNT;
					empty--;
					break;
				}

				if (colors[i] == NO_COLOR)
					c--;
			}


		}

		mb_net_match_handler_send_penality_bubbles(priv->handler,
							   p->con,
							   p->handler_id,
							   colors);
		p->waiting_bubbles = colors;
	}

	shooter_set_angle(monkey_get_shooter(p->monkey), radian);

	monkey_update(p->monkey, time);
	monkey_shoot(p->monkey, time);
	g_mutex_unlock(priv->players_mutex);
}


static Color what_color(Color * colors_count)
{
	gint rnd;
	Color count;



	rnd = rand() % COLORS_COUNT;
	count = 0;
	while (rnd >= 0) {
		count++;
		count %= COLORS_COUNT;

		while (colors_count[count] == 0) {
			count++;
			count %= COLORS_COUNT;
		}
		rnd--;
	}

	return count;
}


static void _init_player(MbNetMatch * self, _Player * player,
			 Color * colors, Color bubble1, Color bubble2)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	Monkey *m;
	Shooter *s;

	Bubble **bubbles;
	int i;
	//        Color c;

	m = monkey_new(TRUE);

	bubbles = g_new0(Bubble *, INIT_BUBBLES_COUNT);
	for (i = 0; i < INIT_BUBBLES_COUNT; i++) {
		bubbles[i] = bubble_new(colors[i], 0, 0);
	}

	board_init(playground_get_board(monkey_get_playground(m)),
		   bubbles, INIT_BUBBLES_COUNT);

	s = monkey_get_shooter(m);


	g_signal_connect(G_OBJECT(m),
			 "bubble-sticked",
			 G_CALLBACK(_bubble_sticked), player);



	g_signal_connect(G_OBJECT(m),
			 "game-lost", G_CALLBACK(_game_lost), player);


	g_signal_connect(G_OBJECT(m),
			 "bubbles-exploded",
			 G_CALLBACK(_bubbles_exploded), player);
/*

	g_signal_connect (G_OBJECT (s),
			  "bubble-added", G_CALLBACK (bubble_added), client);


*/
	shooter_add_bubble(s, bubble_new(bubble1, 0, 0));
	shooter_add_bubble(s, bubble_new(bubble2, 0, 0));

	g_signal_connect(priv->handler, "shoot", (GCallback) _shoot, self);
	player->monkey = m;
/*
	g_signal_connect (G_OBJECT
			  (network_client_get_handler (client->client)),
			  "recv-shoot", G_CALLBACK (recv_shoot), client);*/
	mb_net_match_handler_send_match_init(priv->handler, player->con,
					     player->handler_id,
					     INIT_BUBBLES_COUNT, colors,
					     FALSE, bubble1, bubble2);
}


static void _init_players(MbNetMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	Color colors[INIT_BUBBLES_COUNT];
	Color *c;
	int i;
	Color b1, b2;

	for (i = 0; i < INIT_BUBBLES_COUNT; i++) {
		Color c = rand() % COLORS_COUNT;
		colors[i] = c;
	}

	c = g_new(Color, COLORS_COUNT);

	for (i = 0; i < INIT_BUBBLES_COUNT; i++) {
		c[colors[i]]++;
	}

	b1 = what_color(c);
	b2 = what_color(c);

	GList *next = priv->players;

	while (next != NULL) {
		_Player *player;
		player = (_Player *) (next->data);
		_init_player(self, player, colors, b1, b2);
		next = g_list_next(next);
	}
}

static void _update_player(_Player * player, MbNetMatch * self)
{

	Private *priv;
	priv = GET_PRIVATE(self);

	monkey_update(player->monkey, mb_clock_get_time(priv->clock));
}


static gboolean _update_idle(MbNetMatch * self)
{

	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->players_mutex);


	g_list_foreach(priv->players, (GFunc) _update_player, self);

//      game_finished = update_lost (game);
	g_mutex_unlock(priv->players_mutex);

	return TRUE;		//!game_finished;
}

static void _start_match(MbNetMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	_init_players(self);

	g_mutex_lock(priv->players_mutex);
	GList *next = priv->players;

	while (next != NULL) {
		_Player *player;
		player = (_Player *) (next->data);
		mb_net_match_handler_send_start(priv->handler, player->con,
						player->handler_id);
		next = g_list_next(next);
	}
	g_mutex_unlock(priv->players_mutex);

	mb_clock_start(priv->clock);

	g_timeout_add(10, (GSourceFunc) _update_idle, self);

}


static void _remove_player(MbNetMatch * self, MbNetServerPlayer * p)
{

	Private *priv;
	priv = GET_PRIVATE(self);

	g_mutex_lock(priv->players_mutex);

	GList *l = g_list_find(priv->waited_players, p);

	if (l != NULL) {
		priv->waited_players =
		    g_list_delete_link(priv->waited_players, l);
		g_object_unref(p);

	} else {

		_Player *current = NULL;
		GList *next = priv->players;
		while (next != NULL) {
			_Player *c;
			c = (_Player *) (next->data);

			if (c->player == p) {
				current = c;
				break;
			}
			next = g_list_next(next);
		}

		if (current != NULL) {
			current->lost = TRUE;
			current->player = NULL;
			g_object_unref(p);
		}


	}

	g_mutex_unlock(priv->players_mutex);



}

static void
mb_net_match_get_property(GObject * object, guint prop_id, GValue * value,
			  GParamSpec * param_spec)
{
	MbNetMatch *self;

	self = MB_NET_MATCH(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_match_set_property(GObject * object, guint prop_id,
			  const GValue * value, GParamSpec * param_spec)
{
	MbNetMatch *self;

	self = MB_NET_MATCH(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void mb_net_match_class_init(MbNetMatchClass * mb_net_match_class)
{
	GObjectClass *g_object_class;

	parent_class = g_type_class_peek_parent(mb_net_match_class);


	g_type_class_add_private(mb_net_match_class, sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_match_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_match_set_property;
	g_object_class->get_property = mb_net_match_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_match_finalize;


}
