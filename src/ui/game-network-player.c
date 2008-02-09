/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* game.c
 * Copyright (C) 2002 Laurent Belmonte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include "game-network-player.h"
#include "game.h"
#include "monkey-view.h"
#include "mini-view.h"
#include "clock.h"


#include "player-input.h"
#include "input-manager.h"

#include "game-sound.h"
#define FRAME_DELAY 10

#define PRIVATE(game_network_player) (game_network_player->private)


static GObjectClass *parent_class = NULL;

struct WaitingBubbles {	
	guint8 waiting_bubbles_count;
	guint8 * columns;
	guint8 * colors;
	gint time;
};

struct GameNetworkPlayerPrivate
{
	MonkeyCanvas *canvas;
	GtkWidget *window;
	MonkeyView *display;
	Monkey *monkey;
	guint timeout_id;
	GameState state;
	MbClock *clock;

	GMutex *lock;
	gboolean lost;

	Bubble **waiting_range;
	gint score;
	Block *paused_block;
	Layer *paused_layer;

        guint last_shoot;
	NetworkMessageHandler *handler;
        MbMiniView * mini_views[4];
	int monkey_id;
        struct WaitingBubbles * wb;
        Bubble ** waiting_bubbles;
	MbPlayerInput *input;
        gboolean canShoot;
        gboolean bubbleSticked;
        guint last_sticked;

};



static void 
_add_bubbles(GameNetworkPlayer * game);

static void
recv_add_row (NetworkMessageHandler * handler,
		 int monkey_id, Color * colors, GameNetworkPlayer * game);
static void recv_winlost (NetworkMessageHandler * handler,
			  int monkey_id,
			  gboolean winlost, GameNetworkPlayer * game);

static void game_network_player_bubble_sticked (Monkey * monkey, Bubble * b,
						GameNetworkPlayer * game);

static void game_network_player_game_lost (Monkey * monkey,
					   GameNetworkPlayer * game);

static void game_network_player_bubble_shot (Monkey * monkey,
					     Bubble * bubble,
					     GameNetworkPlayer * game);


static void game_network_player_start (Game * game);
static void game_network_player_stop (Game * game);
static void game_network_player_pause (Game * game, gboolean pause);

static GameState game_network_player_get_state (Game * game);

void game_network_player_fire_changed (GameNetworkPlayer * game);



static gint game_network_player_timeout (gpointer data);

static gboolean released (MbPlayerInput * i,
			  gint key, GameNetworkPlayer * game);

static gboolean pressed (MbPlayerInput * i,
			 gint key, GameNetworkPlayer * game);


static gint get_time (GameNetworkPlayer * game);
static void time_paused (GameNetworkPlayer * game);
static void time_init (GameNetworkPlayer * game);

G_DEFINE_TYPE (GameNetworkPlayer, game_network_player, TYPE_GAME);

static void
game_network_player_init (GameNetworkPlayer* game)
{
	game->private = g_new0 (GameNetworkPlayerPrivate, 1);
	PRIVATE (game)->lock = g_mutex_new ();
}

static void
game_network_player_finalize (GObject * object)
{

	GameNetworkPlayer *game = GAME_NETWORK_PLAYER (object);

	gtk_timeout_remove (PRIVATE (game)->timeout_id);

	g_signal_handlers_disconnect_by_func (G_OBJECT
					      (PRIVATE (game)->input),
					      GTK_SIGNAL_FUNC (pressed),
					      game);

	g_signal_handlers_disconnect_by_func (G_OBJECT
					      (PRIVATE (game)->input),
					      GTK_SIGNAL_FUNC (released),
					      game);


	g_signal_handlers_disconnect_matched (G_OBJECT
					      (PRIVATE (game)->monkey),
					      G_SIGNAL_MATCH_DATA, 0, 0, NULL,
					      NULL, game);



	g_signal_handlers_disconnect_matched (G_OBJECT
					      (PRIVATE (game)->handler),
					      G_SIGNAL_MATCH_DATA, 0, 0, NULL,
					      NULL, game);

#ifdef DEBUG
        g_print("finalized\n");
#endif
	g_object_unref (PRIVATE (game)->clock);
	g_object_unref (PRIVATE (game)->display);

	g_mutex_lock (PRIVATE (game)->lock);
	g_mutex_unlock (PRIVATE (game)->lock);
	g_object_unref (PRIVATE (game)->monkey);
	g_free (game->private);

	if (G_OBJECT_CLASS (parent_class)->finalize)
	{
		(*G_OBJECT_CLASS (parent_class)->finalize) (object);
	}

}


static void
game_network_player_class_init (GameNetworkPlayerClass * klass)
{

	GObjectClass *object_class;
	GameClass *game_class;
	parent_class = g_type_class_peek_parent (klass);
	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = game_network_player_finalize;

	game_class = &(klass->parent_class);
	game_class->start = game_network_player_start;
	game_class->stop = game_network_player_stop;
	game_class->pause = game_network_player_pause;
	game_class->get_state = game_network_player_get_state;

}

static void
game_network_player_bubble_sticked (Monkey * monkey, Bubble * b,
				    GameNetworkPlayer * game)
{


	g_assert (IS_GAME_NETWORK_PLAYER (game));

        PRIVATE(game)->bubbleSticked = TRUE;
        PRIVATE(game)->last_sticked = get_time(game);
        if( PRIVATE(game)->waiting_bubbles != NULL ) {
                _add_bubbles(game);
        }
	if ((monkey_get_shot_count (monkey) % 8) == 0)
	{
		monkey_insert_bubbles (monkey, PRIVATE(game)->waiting_range);

	}


}

gboolean
add_bubble (gpointer ud)
{
	Monkey *monkey;
	monkey = (Monkey *) ud;
	return FALSE;
}


static void
recv_add_bubble (NetworkMessageHandler * handler,
		 int monkey_id, Color color, GameNetworkPlayer * game)
{
	Monkey *monkey;

	g_assert (IS_GAME_NETWORK_PLAYER (game));

	if (PRIVATE (game)->state == GAME_PLAYING)
	{
		g_mutex_lock (PRIVATE (game)->lock);
		monkey = PRIVATE (game)->monkey;
		shooter_add_bubble (monkey_get_shooter (monkey),
				    bubble_new (color, 0, 0));
		g_mutex_unlock (PRIVATE (game)->lock);
	}
}

static void
recv_winlost (NetworkMessageHandler * handler,
	      int monkey_id, gboolean winlost, GameNetworkPlayer * game)
{

	g_assert (IS_GAME_NETWORK_PLAYER (game));

#ifdef DEBUG
	g_print ("GameNetworkPlayer: winlost %d\n", winlost);
#endif
	g_mutex_lock (PRIVATE (game)->lock);
	if (winlost == FALSE)
	{

		PRIVATE (game)->state = GAME_FINISHED;

		PRIVATE (game)->score++;
		monkey_view_draw_win (PRIVATE (game)->display);



		game_network_player_fire_changed (game);

	}
	else
	{

		PRIVATE (game)->state = GAME_FINISHED;

		monkey_view_draw_lost (PRIVATE (game)->display);


		game_network_player_fire_changed (game);

	}
	g_mutex_unlock (PRIVATE (game)->lock);


}

static void
recv_waiting_added (NetworkMessageHandler * handler,
		    int monkey_id,
                    int time,
		    int bubbles_count, GameNetworkPlayer * game)
{

	Monkey *monkey;

	g_assert (IS_GAME_NETWORK_PLAYER (game));

	monkey = PRIVATE (game)->monkey;

	g_mutex_lock (PRIVATE (game)->lock);
        g_print("add bubbles %d \n",bubbles_count);
        monkey_add_bubbles (monkey, bubbles_count);
	g_mutex_unlock (PRIVATE (game)->lock);

}


static void
recv_score(NetworkMessageHandler * handler,
		  guint32 monkey_id,
		  guint8 score,
		  GameNetworkPlayer * game)
{

    
        g_mutex_lock (PRIVATE (game)->lock);
        
        if( monkey_id >= 1 && monkey_id <= 4 ) {
                mb_mini_view_set_score(PRIVATE(game)->mini_views[monkey_id -1],score);
        }
        g_mutex_unlock (PRIVATE (game)->lock);

    
}

static void
recv_bubble_array(NetworkMessageHandler * handler,
		  guint32 monkey_id,
		  guint8 bubble_count,
		  Color * colors,
                  guint32 odd,
		  GameNetworkPlayer * game)
{

    
        g_mutex_lock (PRIVATE (game)->lock);
        
        if( monkey_id >= 1 && monkey_id <= 4 ) {
                mb_mini_view_update(PRIVATE(game)->mini_views[monkey_id -1],
                                    colors,odd);
        }
        g_mutex_unlock (PRIVATE (game)->lock);

    
}


static void
recv_next_range (NetworkMessageHandler * handler,
		 int monkey_id, Color * colors, GameNetworkPlayer * game)
{

	Bubble **bubbles;
	int i;

	g_assert (IS_GAME_NETWORK_PLAYER (game));

	bubbles = g_malloc (sizeof (Bubble *) * 8);

	for (i = 0; i < 8; i++)
	{
                if( colors[i] != NO_COLOR) {
        		bubbles[i] = bubble_new (colors[i], 0, 0);
                }
	}


	PRIVATE (game)->waiting_range = bubbles;
}


static void 
_add_bubbles(GameNetworkPlayer * game)
{
        monkey_add_waiting_row_complete(PRIVATE(game)->monkey, PRIVATE(game)->waiting_bubbles);
        PRIVATE(game)->waiting_bubbles = NULL;
        PRIVATE(game)->bubbleSticked = FALSE;
        PRIVATE(game)->canShoot = TRUE;
}


static void
recv_add_row (NetworkMessageHandler * handler,
		 int monkey_id, Color * colors, GameNetworkPlayer * game)
{

	Bubble **bubbles;
	int i;

	g_assert (IS_GAME_NETWORK_PLAYER (game));

	bubbles = g_malloc (sizeof (Bubble *) * 7);

	for (i = 0; i < 7; i++)
	{
                if( colors[i] != NO_COLOR) {
        		bubbles[i] = bubble_new (colors[i], 0, 0);
                } else {
                        bubbles[i] = NULL;
                }
	}

        g_print("add row recieve ! \n");

	PRIVATE (game)->waiting_bubbles = bubbles;

        if( PRIVATE(game)->bubbleSticked == TRUE ) {
                _add_bubbles(game);
        }
}

static void
game_network_player_bubble_shot (Monkey * monkey,
				 Bubble * bubble, GameNetworkPlayer * game)
{




	g_assert (IS_GAME_NETWORK_PLAYER (game));

	if (PRIVATE (game)->state == GAME_PLAYING)
	{

                PRIVATE(game)->last_shoot = get_time(game);
                PRIVATE(game)->bubbleSticked = FALSE;
                if( monkey_get_waiting_bubbles_count(monkey) > 0 ) {
                        PRIVATE(game)->canShoot = FALSE;
                        

                }
		network_message_handler_send_shoot (PRIVATE (game)->handler,
						    PRIVATE (game)->monkey_id,
						    get_time (game),
						    shooter_get_angle
						    (monkey_get_shooter
						     (monkey)));

               
	}
}


GameNetworkPlayer *
game_network_player_new (GtkWidget * window, MonkeyCanvas * canvas,
			 Monkey * m, NetworkMessageHandler * handler,
			 int monkey_id, int score)
{
	GameNetworkPlayer *game;
        MbGameSound * mgs;
        int i,x,y;
	MbInputManager *input_manager;
	game = GAME_NETWORK_PLAYER (g_object_new
				    (TYPE_GAME_NETWORK_PLAYER, NULL));


	monkey_canvas_clear (canvas);
	PRIVATE (game)->monkey = m;
	PRIVATE (game)->display =
		monkey_view_new (canvas, PRIVATE (game)->monkey, -175, 0,
                                 DATADIR"/monkey-bubble/gfx/layout_network_player.svg",
                                 TRUE,
                                 FALSE);

	PRIVATE (game)->canvas = canvas;
	PRIVATE (game)->window = window;
	PRIVATE (game)->handler = handler;

	g_signal_connect (G_OBJECT (handler), "recv-add-bubble",
			  G_CALLBACK (recv_add_bubble), game);

	g_signal_connect (G_OBJECT (handler), "recv-winlost",
			  G_CALLBACK (recv_winlost), game);

	g_signal_connect (G_OBJECT (handler), "recv-waiting-added",
			  G_CALLBACK (recv_waiting_added), game);

	g_signal_connect (G_OBJECT (handler), "recv-add-row",
			  G_CALLBACK (recv_add_row), game);

        g_signal_connect( G_OBJECT( handler), "recv-bubble-array",
                          G_CALLBACK(recv_bubble_array),game);


        g_signal_connect( G_OBJECT( handler), "recv-score",
                          G_CALLBACK(recv_score),game);

	g_signal_connect (G_OBJECT (handler), "recv-next-range",
			  G_CALLBACK (recv_next_range), game);

	monkey_view_set_score (PRIVATE (game)->display, score);



	PRIVATE (game)->paused_block =
		monkey_canvas_create_block_from_image (canvas,
						       DATADIR
						       "/monkey-bubble/gfx/pause.svg",
						       200, 200, 100, 100);

	PRIVATE (game)->paused_layer =
		monkey_canvas_append_layer (canvas, 0, 0);

	PRIVATE (game)->monkey_id = monkey_id;

	PRIVATE (game)->clock = mb_clock_new ();
	PRIVATE (game)->timeout_id =
		gtk_timeout_add (FRAME_DELAY, game_network_player_timeout,
				 game);


	PRIVATE (game)->state = GAME_STOPPED;

	PRIVATE (game)->lost = FALSE;
	PRIVATE (game)->canShoot = TRUE;
        PRIVATE(game)->waiting_bubbles=  NULL;
	PRIVATE (game)->score = score;
	monkey_view_set_points (PRIVATE (game)->display, score);

	g_signal_connect (G_OBJECT (PRIVATE (game)->monkey),
			  "bubble-sticked",
			  G_CALLBACK (game_network_player_bubble_sticked),
			  game);

	g_signal_connect (G_OBJECT (PRIVATE (game)->monkey),
			  "game-lost",
			  G_CALLBACK (game_network_player_game_lost), game);


	g_signal_connect (G_OBJECT (PRIVATE (game)->monkey),
			  "bubble-shot",
			  G_CALLBACK (game_network_player_bubble_shot), game);



        mgs = mb_game_sound_new();
        mb_game_sound_connect_monkey(mgs,PRIVATE(game)->monkey);

	input_manager = mb_input_manager_get_instance ();
	PRIVATE (game)->input = mb_input_manager_get_left (input_manager);

	g_signal_connect (PRIVATE (game)->input, "notify-pressed",
			  GTK_SIGNAL_FUNC (pressed), game);

	g_signal_connect (PRIVATE (game)->input, "notify-released",
			  GTK_SIGNAL_FUNC (released), game);


        x=350;
        y=30;
        for(i = 0; i < 4; i++) {

                if( i == 1) {
                        x +=155;
                }               
                if( i == 2) {
                        x -=155;
                        y+=210;
                }

                if( i == 3) {
                        x+=155;
                }

                PRIVATE(game)->mini_views[i]  =
                        mb_mini_view_new( canvas,x,y);


        }

	return game;
}


static gboolean
pressed (MbPlayerInput * i, gint key, GameNetworkPlayer * game)
{

	Monkey *monkey;


	if (PRIVATE (game)->state == GAME_PLAYING)
	{
		monkey = PRIVATE (game)->monkey;
		g_mutex_lock (PRIVATE (game)->lock);
		if (key == LEFT_KEY)
		{
			monkey_left_changed (monkey, TRUE, get_time (game));
		}
		else if (key == RIGHT_KEY)
		{
			monkey_right_changed (monkey, TRUE, get_time (game));
		}
		else if (key == SHOOT_KEY)
		{
                        if( PRIVATE(game)->canShoot == TRUE) {        		
                                monkey_shoot (monkey, get_time (game));                        
                        }
		}
		g_mutex_unlock (PRIVATE (game)->lock);
	}

	return FALSE;
}


static gboolean
released (MbPlayerInput * i, gint key, GameNetworkPlayer * game)
{

	Monkey *monkey;


	if (PRIVATE (game)->state == GAME_PLAYING)
	{
		monkey = PRIVATE (game)->monkey;
		g_mutex_lock (PRIVATE (game)->lock);
		if (key == LEFT_KEY)
		{
			monkey_left_changed (monkey, FALSE, get_time (game));
		}
		else if (key == RIGHT_KEY)
		{
			monkey_right_changed (monkey, FALSE, get_time (game));
		}
		g_mutex_unlock (PRIVATE (game)->lock);
	}

	return FALSE;
}

static gint
game_network_player_timeout (gpointer data)
{


	GameNetworkPlayer *game;
	Monkey *monkey;
	gint time;

	game = GAME_NETWORK_PLAYER (data);
	//    if( PRIVATE(game)->state == GAME_STOPPED) return FALSE;
	monkey = PRIVATE (game)->monkey;

	time = get_time (game);
	if (PRIVATE (game)->state == GAME_PLAYING)
	{
                struct WaitingBubbles * wb = NULL;
		g_mutex_lock (PRIVATE (game)->lock);

                wb = PRIVATE(game)->wb;
                if( wb != NULL && wb->time < time) {
                    //    monkey_add_bubbles_at (monkey,wb->waiting_bubbles_count, wb->colors, wb->columns);
	                g_free(wb->colors);
                        g_free(wb->columns);
                        g_free(wb);
                        PRIVATE(game)->wb = NULL;
                }

                
	        if( (time - PRIVATE(game)->last_sticked) > 10000) {
                         if( PRIVATE(game)->canShoot == TRUE) {        		
                                monkey_shoot (monkey, get_time (game));                        
                        } 
	        } 
		monkey_update (monkey, time);

		g_mutex_unlock (PRIVATE (game)->lock);


	}

	monkey_view_update (PRIVATE (game)->display, time);

	monkey_canvas_paint (PRIVATE (game)->canvas);
	return TRUE;
}

void game_network_player_set_start_time(GameNetworkPlayer * g,GTimeVal start_time)
{
        mb_clock_set_reference_time(PRIVATE(g)->clock,start_time);
}

static gint
get_time (GameNetworkPlayer * game)
{
	return mb_clock_get_time (PRIVATE (game)->clock);
}


static void
time_paused (GameNetworkPlayer * game)
{
	mb_clock_pause (PRIVATE (game)->clock, TRUE);
}

static void
time_unpaused (GameNetworkPlayer * game)
{
	mb_clock_pause (PRIVATE (game)->clock, FALSE);
}

static void
time_init (GameNetworkPlayer * game)
{
	mb_clock_start (PRIVATE (game)->clock);
}

static void
game_network_player_start (Game * game)
{
	GameNetworkPlayer *g;

	g_assert (IS_GAME_NETWORK_PLAYER (game));

	g = GAME_NETWORK_PLAYER (game);

	time_init (g);

	PRIVATE (g)->state = GAME_PLAYING;
}

static void
game_network_player_stop (Game * game)
{
	GameNetworkPlayer *g;

	g_assert (IS_GAME_NETWORK_PLAYER (game));

	g = GAME_NETWORK_PLAYER (game);

	PRIVATE (g)->state = GAME_STOPPED;

	//  time_paused(g);
	game_network_player_fire_changed (g);

}

gboolean
game_network_player_is_lost (GameNetworkPlayer * g)
{

	g_assert (GAME_NETWORK_PLAYER (g));

	return PRIVATE (g)->lost;
}


gint
game_network_player_get_score (GameNetworkPlayer * g)
{

	g_assert (GAME_NETWORK_PLAYER (g));

	return PRIVATE (g)->score;
}

static void
game_network_player_pause (Game * game, gboolean pause)
{
	GameNetworkPlayer *g;
	g_assert (IS_GAME_NETWORK_PLAYER (game));

	g = GAME_NETWORK_PLAYER (game);

	if (pause)
	{
		PRIVATE (g)->state = GAME_PAUSED;
		time_paused (g);

		monkey_canvas_add_block (PRIVATE (g)->canvas,
					 PRIVATE (g)->paused_layer,
					 PRIVATE (g)->paused_block, 320, 240);

		game_network_player_fire_changed (g);
	}
	else
	{
		PRIVATE (g)->state = GAME_PLAYING;
		time_unpaused (g);
		monkey_canvas_remove_block (PRIVATE (g)->canvas,
					    PRIVATE (g)->paused_block);

		game_network_player_fire_changed (g);

	}
}

static GameState
game_network_player_get_state (Game * game)
{
	GameNetworkPlayer *g;
	g_assert (IS_GAME_NETWORK_PLAYER (game));

	g = GAME_NETWORK_PLAYER (game);

	return PRIVATE (g)->state;
}

static void
game_network_player_game_lost (Monkey * monkey, GameNetworkPlayer * g)
{
	g_assert (IS_GAME_NETWORK_PLAYER (g));


	PRIVATE (g)->lost = TRUE;
}



void
game_network_player_fire_changed (GameNetworkPlayer * game)
{
	game_notify_changed (GAME (game));
}
