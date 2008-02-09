/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* this file is part of criawips a gnome presentation application
 *
 * AUTHORS
 *       Laurent Belmonte        <laurent.belmonte@aliacom.fr>
 *
 * Copyright (C) 2004 Laurent Belmonte
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

#include <glib-object.h>
#include <glib.h>

#include <string.h>

#include "game.h"
#include "clock.h"
#include "monkey.h"
#include "client.h"


#define INIT_BUBBLES_COUNT 8+7+8+7

#define PRIVATE(self) (self->private )
static GObjectClass *parent_class = NULL;


enum
{
	GAME_STOPPED,
	LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL];



struct NetworkGamePrivate
{
	GList *clients;

	GList * observers;

	GList *lost_clients;
	GMutex *clients_lock;

	GMutex *observers_lock;
	GMutex *lost_clients_lock;
	MbClock *clock;
	guint client_idle_id;
};

struct Client
{
	Monkey *monkey;
	NetworkClient *client;
	NetworkGame *game;
	guint8 *waiting_range;
	Bubble **waiting_bubbles_range;
	Color * waiting_bubbles;
	GMutex *monkey_lock;
	gboolean playing;
	guint score;
};



static void init_clients (NetworkGame * self);
static void client_disconnected (NetworkClient * c, struct Client *client);

static void remove_client (NetworkGame * self, struct Client *client);

static void remove_observer(NetworkGame * self, NetworkClient * observer);
static void notify_observers (NetworkGame * self, struct Client *client);


static gboolean
update_client_idle (gpointer d);

G_DEFINE_TYPE (NetworkGame, network_game, G_TYPE_OBJECT);

static void
client_disconnected (NetworkClient * c, struct Client *client)
{

	NetworkGame *self;

	self = client->game;

	g_mutex_lock (PRIVATE (self)->clients_lock);

	g_assert (NETWORK_IS_CLIENT (c));
	g_assert (NETWORK_IS_CLIENT (client->client));
	remove_client (client->game, client);

	g_mutex_unlock (PRIVATE (self)->clients_lock);

}


static void
observer_disconnected (NetworkClient * c,NetworkGame * self)
{


	g_mutex_lock (PRIVATE (self)->observers_lock);

	remove_observer (self, c);

	g_mutex_unlock (PRIVATE (self)->clients_lock);

}

static void
add_client (gpointer data, gpointer user_data)
{

	NetworkClient *client;
	NetworkGame *game;
	struct Client *c;

	client = NETWORK_CLIENT (data);
	game = NETWORK_GAME (user_data);

	c = g_new0 (struct Client,1);

	c->playing = FALSE;
	c->client = client;

	g_object_ref (G_OBJECT (client));

	c->game = game;
	c->score = 0;
	c->monkey_lock = g_mutex_new ();

	c->monkey = monkey_new (TRUE);


	g_signal_connect (G_OBJECT (client), "disconnected",
			  G_CALLBACK (client_disconnected), c);


	PRIVATE (game)->clients = g_list_append (PRIVATE (game)->clients, c);



}

static void
add_observer (gpointer data, gpointer user_data)
{

	NetworkClient *client;
	NetworkGame *game;

	client = NETWORK_CLIENT (data);
	game = NETWORK_GAME (user_data);


	g_object_ref (G_OBJECT (client));


	g_signal_connect (G_OBJECT (client), "disconnected",
			  G_CALLBACK (observer_disconnected), game);


	PRIVATE (game)->observers = g_list_append (PRIVATE (game)->observers,client);



}


static void
add_bubble (struct Client *c)
{
	gint *colors_count;
	gint rnd, count;
	Monkey *monkey;


	monkey = c->monkey;
	colors_count =
		board_get_colors_count (playground_get_board
					(monkey_get_playground (monkey)));

	rnd = rand () % COLORS_COUNT;
	count = 0;
	while (rnd >= 0)
	{
		count++;
		count %= COLORS_COUNT;

		while (colors_count[count] == 0)
		{
			count++;
			count %= COLORS_COUNT;
		}
		rnd--;
	}

	shooter_add_bubble (monkey_get_shooter (monkey),
			    bubble_new (count, 0, 0));

}

static void
recv_shoot (NetworkMessageHandler * handler,
	    guint32 monkey_id, guint32 time, gfloat angle, struct Client *c)
{


	g_mutex_lock (c->monkey_lock);
	if (c->playing == TRUE)
	{

		if(monkey_get_waiting_bubbles_count( c->monkey) > 0 ) {
		int i;
		g_print("receive shoot \n");

		Color * colors = g_new0( Color,7);
		for( i = 0; i < 7; i++) {
			colors[i] = NO_COLOR;
		}
		
		int count = MIN( monkey_get_waiting_bubbles_count( c->monkey),7);
		int empty = 7;
		int j;

		for( j = 0; j < count; j++ ) {
			int c = rand()% empty;
			int i;
			for( i = 0; i < 7; i++) {
				if( (c <= 0) && ( colors[i] == NO_COLOR) ) {
					
					colors[ i ] = rand()%COLORS_COUNT;
					empty--;
					break;
				}

				if( colors[i] == NO_COLOR) c--;
			}

			
		}

		
		network_message_handler_send_add_row(
			network_client_get_handler(c->client),
			network_client_get_id (c->client),
			colors);
		
		c->waiting_bubbles = colors;
		}
		shooter_set_angle (monkey_get_shooter (c->monkey), angle);

		monkey_update (c->monkey, time);
		monkey_shoot (c->monkey, time);
	}
	g_mutex_unlock (c->monkey_lock);
}


static void
bubble_added (Shooter * s, Bubble * b, struct Client *c)
{

	network_message_handler_send_add_bubble (network_client_get_handler
						 (c->client),
						 network_client_get_id (c->
									client),
						 bubble_get_color (b));




}

static void
add_range_to_client (struct Client *c)
{

	guint8 *colors;
	Bubble **bubbles;
	int i;

	colors = g_malloc (sizeof (Color) * 8);
	bubbles = g_malloc (sizeof (Bubble *) * 8);
	for (i = 0; i < 8; i++)
	{
		colors[i] = rand () % COLORS_COUNT;
		bubbles[i] = bubble_new (colors[i], 0, 0);
	}

	c->waiting_range = colors;
	c->waiting_bubbles_range = bubbles;
	network_message_handler_send_next_range (network_client_get_handler
						 (c->client),
						 network_client_get_id (c->
									client),
						 colors);




}

static void
bubble_sticked (Monkey * monkey, Bubble * b, struct Client *c)
{



	if( c->waiting_bubbles != NULL ) {
		int i;
		Bubble ** bubbles = g_new0(Bubble *,7);
		for(i=0; i < 7; i++) {
			if( c->waiting_bubbles[i] != NO_COLOR) {
				bubbles[i] = bubble_new(c->waiting_bubbles[i],0,0);
			}
		}
		monkey_add_waiting_row_complete(monkey,bubbles);
		g_free(c->waiting_bubbles);
		c->waiting_bubbles = NULL;
	}
	if (!monkey_is_empty (monkey))
	{
		add_bubble (c);
	}

	if ((monkey_get_shot_count (monkey) % 8) == 1)
	{
		add_range_to_client (c);
	}

	if ((monkey_get_shot_count (monkey) % 8) == 0)
	{

		monkey_insert_bubbles (monkey, c->waiting_bubbles_range);
		c->waiting_range = NULL;
		//add_range_to_client (c);

	}

	notify_observers (c->game, c);
#ifdef DEBUG
	monkey_print_board (monkey);
#endif
}

static void
notify_observers (NetworkGame * self, struct Client *client)
{

	Bubble **bubbles;
	Color *colors;
	int count, i;
	gboolean odd;
	Board * board;
	GList *next;

	board = playground_get_board(monkey_get_playground (client->monkey));
	bubbles =
		board_get_array (board);

	odd = board_get_odd(board);
	count = 8 * 13;
	colors = g_malloc (sizeof (Color) * count);

	for (i = 0; i < count; i++)
	{

		if (bubbles[i] != NULL)
		{
			colors[i] = bubble_get_color (bubbles[i]);
		}
		else
		{
			colors[i] = NO_COLOR;
		}
	}

	g_mutex_lock( PRIVATE(self)->observers_lock);
	next = PRIVATE (self)->observers;

	while (next != NULL)
	{
		NetworkClient * nc;

		nc = NETWORK_CLIENT(next->data);
		network_message_handler_send_bubble_array
			(network_client_get_handler (nc),
			 network_client_get_id (client->client),
			 8 * 13,
			 colors,
			 odd);

	network_message_handler_send_score(network_client_get_handler (nc),
			 network_client_get_id (client->client),
			network_client_get_score(client->client));

		next = g_list_next (next);
	}


	g_mutex_unlock( PRIVATE(self)->observers_lock);

}

static void
bubbles_exploded (Monkey * monkey,
		  GList * exploded, GList * fallen, struct Client *c)
{


	int to_go;
	GList *next;

	to_go = MAX (0,
		     -3 + g_list_length (exploded) +
		     g_list_length (fallen) * 1.5);

	if (to_go != 0)
	{

		next = PRIVATE (c->game)->clients;

		while (next != NULL)
		{
			struct Client *client;
			Monkey *other;

			client = (struct Client *) next->data;

			if (client != c)
			{
				g_mutex_lock (client->monkey_lock);
				other = client->monkey;

				monkey_add_bubbles(other,to_go);
				network_message_handler_send_waiting_added
					(network_client_get_handler
					 (client->client),					 
					 network_client_get_id (client->
								client),
					 mb_clock_get_time (PRIVATE (c->game)->clock),
					 to_go);
				g_mutex_unlock (client->monkey_lock);

			}
			next = g_list_next (next);
		}


	}

}


static void
game_lost (Monkey * m, struct Client *c)
{

	NetworkGame *game;

	game = c->game;
#ifdef DEBUG
	g_print ("network-game : lost ,player id %d\n",
		 network_client_get_id (c->client));
#endif
	g_mutex_lock (PRIVATE (game)->lost_clients_lock);

	network_message_handler_send_winlost (network_client_get_handler
					      (c->client),
					      network_client_get_id (c->
								     client),
					      1);

	c->playing = FALSE;
	PRIVATE (game)->lost_clients =
		g_list_append (PRIVATE (game)->lost_clients, c);

	g_mutex_unlock (PRIVATE (game)->lost_clients_lock);


}


static void
init_client (NetworkGame * self, struct Client *client,
	     Color * colors, Color b1, Color b2, int count)
{
	Monkey *m;
	Shooter *s;

	Bubble **bubbles;
	int i;
	//        Color c;

	m = client->monkey;

	bubbles = g_malloc (sizeof (Bubble *) * count);


	for (i = 0; i < count; i++)
	{
		bubbles[i] = bubble_new (colors[i], 0, 0);
	}


	board_init (playground_get_board (monkey_get_playground (m)),
		    bubbles, count);

	network_message_handler_send_bubble_array (network_client_get_handler
						   (client->client),
						   network_client_get_id
						   (client->client),
						   INIT_BUBBLES_COUNT,
						   colors,
						   FALSE);

	s = monkey_get_shooter (m);


	g_signal_connect (G_OBJECT (s),
			  "bubble-added", G_CALLBACK (bubble_added), client);



	g_signal_connect (G_OBJECT (m),
			  "bubble-sticked",
			  G_CALLBACK (bubble_sticked), client);



	g_signal_connect (G_OBJECT (m),
			  "game-lost", G_CALLBACK (game_lost), client);


	g_signal_connect (G_OBJECT (m),
			  "bubbles-exploded",
			  G_CALLBACK (bubbles_exploded), client);

	shooter_add_bubble (s, bubble_new (b1, 0, 0));
	shooter_add_bubble (s, bubble_new (b2, 0, 0));

	g_signal_connect (G_OBJECT
			  (network_client_get_handler (client->client)),
			  "recv-shoot", G_CALLBACK (recv_shoot), client);

}


static Color
what_color (int *colors_count)
{
	gint rnd;
	Color count;



	rnd = rand () % COLORS_COUNT;
	count = 0;
	while (rnd >= 0)
	{
		count++;
		count %= COLORS_COUNT;

		while (colors_count[count] == 0)
		{
			count++;
			count %= COLORS_COUNT;
		}
		rnd--;
	}

	return count;
}


static void
init_clients (NetworkGame * self)
{

	GList *next;
	Bubble *bubbles[INIT_BUBBLES_COUNT];
	Color colors[INIT_BUBBLES_COUNT];
	int c[COLORS_COUNT];
	int i;
	Color b1, b2;

	for (i = 0; i < INIT_BUBBLES_COUNT; i++)
	{
		Color c = rand () % COLORS_COUNT;
		colors[i] = c;
		bubbles[i] = bubble_new (c, 0, 0);
	}

	memset (&c, 0, sizeof (c));

	for (i = 0; i < INIT_BUBBLES_COUNT; i++)
	{
		c[colors[i]]++;
	}

	b1 = what_color (c);
	b2 = what_color (c);
	next = PRIVATE (self)->clients;

	while (next != NULL)
	{
		struct Client *client;

		client = (struct Client *) next->data;

		init_client (self, client, colors, b1, b2,
			     INIT_BUBBLES_COUNT);
		next = g_list_next (next);
	}
}

NetworkGame *
network_game_new (GList * clients)
{
	NetworkGame *self;

	self = NETWORK_GAME (g_object_new (NETWORK_TYPE_GAME, NULL));

	g_list_foreach (clients, add_client, self);

	g_list_foreach(clients,add_observer,self);
	init_clients (self);
	return self;
}

static void
idle_stopped (gpointer data)
{

	NetworkGame *g;

	g = NETWORK_GAME (data);

	g_signal_emit (G_OBJECT (g), signals[GAME_STOPPED], 0);
}


static void
send_start (gpointer data, gpointer user_data)
{
	struct Client *client;

	client = data;
	client->playing = TRUE;
	network_message_handler_send_start (network_client_get_handler
					    (client->client));
#ifdef DEBUG
	g_print ("network-game : send_start\n");
#endif
}


static void
update_client (gpointer data, gpointer user_data)
{
	NetworkGame *game;
	struct Client *client;

	client = (struct Client *) data;
	game = NETWORK_GAME (user_data);


	g_mutex_lock (client->monkey_lock);


	monkey_update (client->monkey,
		       mb_clock_get_time (PRIVATE (game)->clock));


	g_mutex_unlock (client->monkey_lock);

}

static gboolean
update_lost (NetworkGame * game)
{

	GList *next;
	gboolean finished;


	g_mutex_lock (PRIVATE (game)->lost_clients_lock);


	finished = FALSE;
	next = PRIVATE (game)->lost_clients;

	while (next != NULL)
	{
		struct Client *c;

		c = (struct Client *) next->data;


		remove_client (game, c);

		next = g_list_next (next);
	}


	g_list_free (PRIVATE (game)->lost_clients);
	PRIVATE (game)->lost_clients = NULL;




	if (g_list_length (PRIVATE (game)->clients) == 1)
	{
		struct Client *client;

		// the client has won the game !
		client = (struct Client *) PRIVATE (game)->clients->data;
		network_client_win(client->client);
		network_message_handler_send_winlost
			(network_client_get_handler (client->client),
			 network_client_get_id (client->client), 0);
		remove_client (game, client);

		finished = TRUE;
	}



	g_mutex_unlock (PRIVATE (game)->lost_clients_lock);

	return finished;
}

static gboolean
update_idle (gpointer d)
{

	NetworkGame *game;
	gboolean game_finished;

	game = NETWORK_GAME (d);

	g_mutex_lock (PRIVATE (game)->clients_lock);



	g_list_foreach (PRIVATE (game)->clients, update_client, game);

	game_finished = update_lost (game);
	g_mutex_unlock (PRIVATE (game)->clients_lock);


	return !game_finished;
}

static gboolean
update_client_idle (gpointer d)
{


	NetworkGame *self;
	gboolean game_finished;

	self = NETWORK_GAME (d);

	g_mutex_lock (PRIVATE (self)->clients_lock);


	GList * next = PRIVATE(self)->clients;
	while(next != NULL) {
		notify_observers(self,(struct Client *)next->data);
		next = g_list_next(next);
	}

	game_finished = update_lost (self);
	g_mutex_unlock (PRIVATE (self)->clients_lock);


	return !game_finished;
}

void
network_game_start (NetworkGame * self)
{
	g_list_foreach (PRIVATE (self)->clients, send_start, self);

	mb_clock_start (PRIVATE (self)->clock);

	g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE,
			    10, update_idle, self, idle_stopped);

	PRIVATE(self)->client_idle_id = g_timeout_add (1000, update_client_idle, self);
	

}

static void
network_game_init (NetworkGame* self)
{
	PRIVATE (self) = g_new0 (NetworkGamePrivate, 1);
	PRIVATE (self)->clients = NULL;
	PRIVATE (self)->clients_lock = g_mutex_new ();


	PRIVATE (self)->observers = NULL;
	PRIVATE (self)->observers_lock = g_mutex_new ();

	PRIVATE (self)->lost_clients = NULL;
	PRIVATE (self)->lost_clients_lock = g_mutex_new ();
	PRIVATE (self)->clock = mb_clock_new ();
}

static void
remove_observer(NetworkGame * self, NetworkClient * observer)
{



	PRIVATE (self)->observers =
		g_list_remove (PRIVATE (self)->observers, observer);

	g_signal_handlers_disconnect_matched (network_client_get_handler (observer),
					      G_SIGNAL_MATCH_DATA, 0, 0, NULL,
					      NULL, self);


}

static void
remove_client (NetworkGame * self, struct Client *client)
{

	NetworkClient *c;


	c = client->client;
	PRIVATE (self)->clients =
		g_list_remove (PRIVATE (self)->clients, client);

	g_signal_handlers_disconnect_matched (network_client_get_handler (c),
					      G_SIGNAL_MATCH_DATA, 0, 0, NULL,
					      NULL, self);


	g_signal_handlers_disconnect_matched (network_client_get_handler (c),
					      G_SIGNAL_MATCH_DATA, 0, 0, NULL,
					      NULL, client);



	g_signal_handlers_disconnect_matched (G_OBJECT (c),
					      G_SIGNAL_MATCH_DATA, 0, 0, NULL,
					      NULL, self);
	g_signal_handlers_disconnect_matched (G_OBJECT (c),
					      G_SIGNAL_MATCH_DATA, 0, 0, NULL,
					      NULL, client);

	g_signal_handlers_disconnect_matched (client->monkey,
					      G_SIGNAL_MATCH_DATA, 0, 0, NULL,
					      NULL, client);

	g_signal_handlers_disconnect_matched (monkey_get_shooter
					      (client->monkey),
					      G_SIGNAL_MATCH_DATA, 0, 0, NULL,
					      NULL, client);

	g_object_unref (client->monkey);

	g_object_unref (client->client);

	g_mutex_free (client->monkey_lock);
	g_free (client);

}

static void
network_game_finalize (GObject * object)
{
	NetworkGame *self;

	self = NETWORK_GAME (object);

	g_assert (PRIVATE (self)->clients == NULL);

	g_source_remove( PRIVATE(self)->client_idle_id);
	g_list_free (PRIVATE (self)->clients);


	g_object_unref (PRIVATE (self)->clock);
	g_mutex_free (PRIVATE (self)->clients_lock);
	g_mutex_free (PRIVATE (self)->lost_clients_lock);
	g_free (PRIVATE (self));
	if (G_OBJECT_CLASS (parent_class)->finalize)
	{
		(*G_OBJECT_CLASS (parent_class)->finalize) (object);
	}

}


static void
network_game_class_init (NetworkGameClass * network_game_class)
{
	GObjectClass *g_object_class;

	parent_class = g_type_class_peek_parent (network_game_class);

	g_object_class = G_OBJECT_CLASS (network_game_class);
	g_object_class->finalize = network_game_finalize;

	signals[GAME_STOPPED] = g_signal_new ("game-stopped",
					      G_TYPE_FROM_CLASS
					      (network_game_class),
					      G_SIGNAL_RUN_FIRST |
					      G_SIGNAL_NO_RECURSE,
					      G_STRUCT_OFFSET
					      (NetworkGameClass,
					       game_stopped), NULL, NULL,
					      g_cclosure_marshal_VOID__VOID,
					      G_TYPE_NONE, 0, NULL);

}

