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
#include "clock.h"
#include "keyboard-properties.h"

#define FRAME_DELAY 10

#define PRIVATE(game_network_player) (game_network_player->private)


static GObjectClass* parent_class = NULL;

struct GameNetworkPlayerPrivate {
    MonkeyCanvas * canvas;
    GtkWidget * window;
    MonkeyView * display;
    Monkey * monkey;
    guint timeout_id;
    GameState state;
    Clock * clock;

    gboolean lost;

    gint score;
    Block * paused_block;
    Layer * paused_layer;

    GConfClient * gconf_client;

    guint left_key;
    GdkModifierType left_key_modifier;


    guint right_key;
    GdkModifierType right_key_modifier;


    guint shoot_key;
    GdkModifierType shoot_key_modifier;

    MonkeyMessageHandler * handler;
    gint notify_id;

    int monkey_id;
};


static void recv_winlost(MonkeyMessageHandler * handler,
                         int monkey_id,
                         gboolean winlost,
                         GameNetworkPlayer * game);
static void game_network_player_add_to_score(GameNetworkPlayer * g,gint points);

static void game_network_player_bubble_sticked(Monkey * monkey,Bubble * b,GameNetworkPlayer * game);

static void game_network_player_game_lost(Monkey * monkey,GameNetworkPlayer * game);

static void game_network_player_bubbles_exploded(Monkey * monkey,
						 GList * exploded,
						 GList * fallen,
						 GameNetworkPlayer * game);

static void game_network_player_bubble_shot(Monkey * monkey,
					    Bubble * bubble,
					    GameNetworkPlayer * game);


static void game_network_player_start(Game * game);
static void game_network_player_stop(Game * game);
static void game_network_player_pause(Game * game,gboolean pause);

static GameState game_network_player_get_state(Game * game);

void game_network_player_fire_changed(GameNetworkPlayer * game);



static gint game_network_player_timeout (gpointer data);

static gboolean game_network_player_key_released(GtkWidget *widget,
						 GdkEventKey *event,
						 gpointer user_data);

static gboolean game_network_player_key_pressed(GtkWidget *widget,
						GdkEventKey *event,
						gpointer user_data);


static gint get_time(GameNetworkPlayer * game);
static void time_paused(GameNetworkPlayer * game);
static void time_init(GameNetworkPlayer * game);

static void game_network_player_conf_keyboard(GameNetworkPlayer * game);

static void game_network_player_instance_init(GameNetworkPlayer * game) {
    game->private =g_new0 (GameNetworkPlayerPrivate, 1);		
}

static void game_network_player_finalize(GObject* object) {

    GameNetworkPlayer * game = GAME_NETWORK_PLAYER(object);

    gtk_timeout_remove( PRIVATE(game)->timeout_id);

    gconf_client_notify_remove(PRIVATE(game)->gconf_client, PRIVATE(game)->notify_id);
    g_signal_handlers_disconnect_by_func( G_OBJECT(PRIVATE(game)->window) ,
					  GTK_SIGNAL_FUNC (game_network_player_key_pressed),game);


    g_signal_handlers_disconnect_by_func( G_OBJECT(PRIVATE(game)->window) ,
					  GTK_SIGNAL_FUNC (game_network_player_key_released),game);

    g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(game)->monkey ),
					   G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,game);
                                                                                


    g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(game)->handler ),
					   G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,game);
                      

    g_print("GOGOGOGO\n");
    g_object_unref( PRIVATE(game)->clock);
    g_object_unref( PRIVATE(game)->display );


    g_object_unref( PRIVATE(game)->monkey);
    g_free(game->private);
  
    if (G_OBJECT_CLASS (parent_class)->finalize) {
	(* G_OBJECT_CLASS (parent_class)->finalize) (object);
    }

}


static void game_network_player_class_init (GameNetworkPlayerClass *klass) {

    GObjectClass* object_class;
    GameClass * game_class;
    parent_class = g_type_class_peek_parent(klass);
    object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = game_network_player_finalize;

    game_class = &(klass->parent_class);
    game_class->start = game_network_player_start;
    game_class->stop = game_network_player_stop;
    game_class->pause = game_network_player_pause;
    game_class->get_state = game_network_player_get_state;

}


GType game_network_player_get_type(void) {
    static GType game_network_player_type = 0;
    
    if (!game_network_player_type) {
	static const GTypeInfo game_network_player_info = {
	    sizeof(GameNetworkPlayerClass),
	    NULL,           /* base_init */
	    NULL,           /* base_finalize */
	    (GClassInitFunc) game_network_player_class_init,
	    NULL,           /* class_finalize */
	    NULL,           /* class_data */
	    sizeof(GameNetworkPlayer),
	    1,              /* n_preallocs */
	    (GInstanceInitFunc) game_network_player_instance_init,
	};
      
      
	game_network_player_type = g_type_register_static(TYPE_GAME,
							  "GameNetworkPlayer",
							  &game_network_player_info, 0);



    }
    
    return game_network_player_type;
}



static void game_network_player_bubble_sticked(Monkey * monkey,Bubble * b,
					       GameNetworkPlayer * game) {


    g_assert( IS_GAME_NETWORK_PLAYER(game));


    if( ( monkey_get_shot_count( monkey) % 8 ) == 0 ) {
            g_print("go down");
            monkey_set_board_down( monkey);
    }


}


static void
game_network_player_config_notify (GConfClient *client,
                                   guint        cnxn_id,
                                   GConfEntry  *entry,
                                   gpointer     user_data) {

    GameNetworkPlayer * game;


    game = GAME_NETWORK_PLAYER (user_data);
    
    game_network_player_conf_keyboard(game);
  
}

gboolean add_bubble(gpointer ud) {
    Monkey * monkey;
    monkey = (Monkey*)ud;
    g_print(" add bubble \n");
    return FALSE;
}


static void recv_add_bubble(MonkeyMessageHandler * handler,
			    int monkey_id,
			    Color color,
			    GameNetworkPlayer * game) {
    Monkey * monkey;

    g_assert( IS_GAME_NETWORK_PLAYER(game));

    monkey = PRIVATE(game)->monkey;
    g_print("recvieve add bubble \n");
    shooter_add_bubble(monkey_get_shooter(monkey),bubble_new(color,0,0));

    //	   g_idle_add( add_bubble,monkey);
	   
  
}

static gboolean idle_draw_win(gpointer data) {

        GameNetworkPlayer * game;

        game = GAME_NETWORK_PLAYER(data);
        monkey_view_draw_win( PRIVATE(game)->display );

        return FALSE;
}

static void recv_winlost(MonkeyMessageHandler * handler,
                         int monkey_id,
                         gboolean winlost,
			    GameNetworkPlayer * game) {

    g_assert( IS_GAME_NETWORK_PLAYER(game));

    if( winlost == FALSE) {
            g_print("you win\n");

            PRIVATE(game)->state = GAME_STOPPED;
    
            g_idle_add( idle_draw_win,game);
    
            game_network_player_stop(GAME(game));
            game_network_player_fire_changed(game);	
    } else {
    }

    //	   g_idle_add( add_bubble,monkey);
	   
  
}

static void recv_waiting_added(MonkeyMessageHandler * handler,
                               int monkey_id,
                               int bubbles_count,
                               Color * colors,
                               guint8 * columns,
                               GameNetworkPlayer * game) {

        Monkey * monkey;
        
        g_assert( IS_GAME_NETWORK_PLAYER(game));
        
        g_print("waiting added id : %d, count %d \n",monkey_id,bubbles_count);
        monkey = PRIVATE(game)->monkey;
        
        monkey_add_bubbles_at(monkey,bubbles_count,colors,columns);
  
}


static void game_network_player_bubble_shot( Monkey * monkey,
					     Bubble * bubble,
					     GameNetworkPlayer * game) {




    g_assert( IS_GAME_NETWORK_PLAYER(game));
		
    monkey_message_handler_send_shoot(PRIVATE(game)->handler,
				      PRIVATE(game)->monkey_id,
				      get_time(game),
				      shooter_get_angle(monkey_get_shooter(monkey)));
}


GameNetworkPlayer * game_network_player_new(GtkWidget * window,MonkeyCanvas * canvas,Monkey * m,
					    MonkeyMessageHandler * handler,int monkey_id) {
    GameNetworkPlayer * game;


    game = GAME_NETWORK_PLAYER (g_object_new (TYPE_GAME_NETWORK_PLAYER, NULL));


    PRIVATE(game)->gconf_client = gconf_client_get_default ();

    gconf_client_add_dir (PRIVATE(game)->gconf_client, CONF_KEY_PREFIX,
			  GCONF_CLIENT_PRELOAD_NONE, NULL);

    monkey_canvas_clear(canvas);
    PRIVATE(game)->monkey =  m;
    PRIVATE(game)->display = 
	monkey_view_new(canvas, 
			PRIVATE(game)->monkey,0,0,TRUE);
  
    PRIVATE(game)->canvas = canvas;
    PRIVATE(game)->window = window;
    PRIVATE(game)->handler = handler;

    g_signal_connect( G_OBJECT( handler), "recv-add-bubble",
		      G_CALLBACK(recv_add_bubble),game);

    g_signal_connect( G_OBJECT( handler), "recv-winlost",
		      G_CALLBACK(recv_winlost),game);

    g_signal_connect( G_OBJECT( handler), "recv-waiting-added",
		      G_CALLBACK(recv_waiting_added),game);

    monkey_view_set_score(PRIVATE(game)->display,1);
    g_signal_connect( G_OBJECT( window) ,"key-press-event",
		      GTK_SIGNAL_FUNC (game_network_player_key_pressed),game);
  
    g_signal_connect( G_OBJECT( window) ,"key-release-event",
		      GTK_SIGNAL_FUNC (game_network_player_key_released),game);
  
  

    PRIVATE(game)->paused_block = 
	monkey_canvas_create_block_from_image(canvas,
					      DATADIR"/monkey-bubble/gfx/pause.svg",
					      200,200,
					      100,100);

    PRIVATE(game)->paused_layer =
	monkey_canvas_append_layer(canvas,0,0);

    PRIVATE(game)->monkey_id = monkey_id;

    PRIVATE(game)->clock = clock_new();
    PRIVATE(game)->timeout_id = 
	gtk_timeout_add (FRAME_DELAY, game_network_player_timeout, game);
  
  
    PRIVATE(game)->state = GAME_STOPPED;

    PRIVATE(game)->lost = FALSE;
  
    PRIVATE(game)->score = 1;

    monkey_view_set_points( PRIVATE(game)->display,1);

    g_signal_connect( G_OBJECT( PRIVATE(game)->monkey),
		      "bubble-sticked",
		      G_CALLBACK(game_network_player_bubble_sticked),
		      game);

    g_signal_connect( G_OBJECT( PRIVATE(game)->monkey),
		      "game-lost",
		      G_CALLBACK(game_network_player_game_lost),
		      game);

    g_signal_connect( G_OBJECT( PRIVATE(game)->monkey),
		      "bubbles-exploded",
		      G_CALLBACK(game_network_player_bubbles_exploded),
		      game);

    g_signal_connect( G_OBJECT( PRIVATE(game)->monkey),
		      "bubble-shot",
		      G_CALLBACK(game_network_player_bubble_shot),
		      game);




    game_network_player_conf_keyboard(game);

    PRIVATE(game)->notify_id =
	gconf_client_notify_add( PRIVATE(game)->gconf_client,
				 CONF_KEY_PREFIX,
				 game_network_player_config_notify,
				 game,NULL,NULL);
    return game;
}

static void game_network_player_conf_keyboard(GameNetworkPlayer * game) {
    gchar * str;
    guint accel;
    GdkModifierType modifier;
    str = gconf_client_get_string(PRIVATE(game)->gconf_client,CONF_KEY_PREFIX"/player_1_left",NULL);

    if( str != NULL) {
	
	gtk_accelerator_parse (str,
			       &accel,
			       &modifier);

	PRIVATE(game)->left_key = accel;
	PRIVATE(game)->left_key_modifier =modifier;
	g_free(str);
    } else {
	PRIVATE(game)->left_key = 115;
	PRIVATE(game)->left_key_modifier = 0;
	
    }

    str = gconf_client_get_string(PRIVATE(game)->gconf_client,CONF_KEY_PREFIX"/player_1_right",NULL);

    if( str != NULL) {
	
	gtk_accelerator_parse (str,
			       &accel,
			       &modifier);

	PRIVATE(game)->right_key = accel;
	PRIVATE(game)->right_key_modifier =modifier;
	g_free(str);
    } else {
	PRIVATE(game)->right_key = 102;
	PRIVATE(game)->right_key_modifier = 0;
	
    }

    str = gconf_client_get_string(PRIVATE(game)->gconf_client,CONF_KEY_PREFIX"/player_1_shoot",NULL);

    if( str != NULL) {
	
	gtk_accelerator_parse (str,
			       &accel,
			       &modifier);

	PRIVATE(game)->shoot_key = accel;
	PRIVATE(game)->shoot_key_modifier =modifier;
	g_free(str);
    } else {
	PRIVATE(game)->shoot_key = 100;
	PRIVATE(game)->shoot_key_modifier = 0;
	
    }
}



static gboolean game_network_player_key_pressed(GtkWidget *widget,
						GdkEventKey *event,
						gpointer user_data) {
  
  
    GameNetworkPlayer * game;
    Monkey * monkey;
  
    game = GAME_NETWORK_PLAYER(user_data);

    if( PRIVATE(game)->state == GAME_PLAYING) {
	monkey = PRIVATE(game)->monkey;
    
	if( event->keyval ==  PRIVATE(game)->left_key ) {
	    monkey_left_changed( monkey,TRUE,get_time(game));
	}
    
	if( event->keyval ==  PRIVATE(game)->right_key ) {
	    monkey_right_changed( monkey,TRUE,get_time(game));
	}
    
    
	if( event->keyval ==  PRIVATE(game)->shoot_key) {
	    monkey_shoot( monkey,get_time(game));
	}
    }

    return FALSE;
}

static gboolean game_network_player_key_released(GtkWidget *widget,
						 GdkEventKey *event,
						 gpointer user_data) {

    GameNetworkPlayer * game;
    Monkey * monkey;

    game = GAME_NETWORK_PLAYER(user_data);
    monkey = PRIVATE(game)->monkey;

    if( PRIVATE(game)->state == GAME_PLAYING ) {
	if( event->keyval == PRIVATE(game)->left_key) {
	    monkey_left_changed( monkey,FALSE,get_time(game));
	}
    
	if( event->keyval ==   PRIVATE(game)->right_key) {
	    monkey_right_changed( monkey,FALSE,get_time(game));
	}

    }
    return FALSE;

}

static gint game_network_player_timeout (gpointer data)
{


    GameNetworkPlayer * game;
    Monkey * monkey;
    gint time;

    game = GAME_NETWORK_PLAYER(data);
    if( PRIVATE(game)->state == GAME_STOPPED) return FALSE;
    monkey = PRIVATE(game)->monkey;

    time = get_time(game);

    if( PRIVATE(game)->state == GAME_PLAYING ) {
  
  
    
	monkey_update( monkey,
		       time );
    
    
    
    } 
    monkey_view_update( PRIVATE(game)->display,
			time);

    monkey_canvas_paint(PRIVATE(game)->canvas);
 
    return TRUE;
}

static gint get_time(GameNetworkPlayer * game) {
    return clock_get_time(PRIVATE(game)->clock);
}


static void time_paused(GameNetworkPlayer * game) {
    clock_pause( PRIVATE(game)->clock, TRUE);
}

static void time_unpaused(GameNetworkPlayer * game) {
    clock_pause( PRIVATE(game)->clock, FALSE);
}

static void time_init(GameNetworkPlayer * game) {
    clock_start( PRIVATE(game)->clock);
}

static void game_network_player_start(Game * game) {
    GameNetworkPlayer * g;
 
    g_assert( IS_GAME_NETWORK_PLAYER(game));
  
    g = GAME_NETWORK_PLAYER(game);

    time_init( g );

    PRIVATE(g)->state = GAME_PLAYING;
}

static void game_network_player_stop(Game * game) {
    GameNetworkPlayer * g;

    g_assert( IS_GAME_NETWORK_PLAYER(game));
  
    g = GAME_NETWORK_PLAYER(game);

    PRIVATE(g)->state = GAME_STOPPED;

    //  time_paused(g);
    game_network_player_fire_changed(g);

}

gboolean game_network_player_is_lost(GameNetworkPlayer * g) {

    g_assert( GAME_NETWORK_PLAYER(g));

    return PRIVATE(g)->lost;
}


gint game_network_player_get_score(GameNetworkPlayer * g) {

    g_assert( GAME_NETWORK_PLAYER(g));

    return PRIVATE(g)->score;
}

static void game_network_player_pause(Game * game,gboolean pause) {
    GameNetworkPlayer * g;
    g_assert( IS_GAME_NETWORK_PLAYER(game));
  
    g = GAME_NETWORK_PLAYER(game);

   if( pause ) {
	PRIVATE(g)->state = GAME_PAUSED;
	time_paused( g);

	monkey_canvas_add_block(PRIVATE(g)->canvas,
				PRIVATE(g)->paused_layer,
				PRIVATE(g)->paused_block,
				320,240);

	game_network_player_fire_changed(g);
    } else {
	PRIVATE(g)->state = GAME_PLAYING;
	time_unpaused( g);
	monkey_canvas_remove_block(PRIVATE(g)->canvas,
				   PRIVATE(g)->paused_block);    

	game_network_player_fire_changed(g);

    }
}

static GameState game_network_player_get_state(Game * game) {
    GameNetworkPlayer * g;
    g_assert( IS_GAME_NETWORK_PLAYER(game));
  
    g = GAME_NETWORK_PLAYER(game);

    return PRIVATE(g)->state;
}

static void game_network_player_game_lost(Monkey * monkey,GameNetworkPlayer * g) {
    g_assert( IS_GAME_NETWORK_PLAYER(g));


    PRIVATE(g)->lost = TRUE;

    monkey_view_draw_lost( PRIVATE(g)->display );

    game_network_player_stop( GAME(g));


    PRIVATE(g)->state = GAME_STOPPED;
  
    game_network_player_fire_changed(g);
    monkey_canvas_paint(PRIVATE(g)->canvas);
    
}

static void game_network_player_bubbles_exploded(  Monkey * monkey,
						   GList * exploded,
						   GList * fallen,
						   GameNetworkPlayer * g) {

    gint points;

    g_assert( IS_GAME_NETWORK_PLAYER(g));	 


    g_print("exploded %d\n",g_list_length(exploded));
    /**
     * evaluate score :
     * a exploded bubble = 10 pts
     * a fall bubble = 20 pts
     * a minimum of 30 pts to add to score
     */

    points = g_list_length(exploded) + g_list_length(fallen)*2;
    if( points > 3 ) {
	game_network_player_add_to_score(g,points*10);
    }    
  
    if( monkey_is_empty( monkey) ) {
    
	PRIVATE(g)->state = GAME_STOPPED;
    
	monkey_view_draw_win( PRIVATE(g)->display );
    
	game_network_player_fire_changed(g);	
	game_network_player_stop(GAME(g));
    }

  
}

static void game_network_player_add_to_score(GameNetworkPlayer * g,gint points) {
    g_assert( IS_GAME_NETWORK_PLAYER(g));

    PRIVATE(g)->score += points;
    monkey_view_set_points(PRIVATE(g)->display,PRIVATE(g)->score);  
}


void game_network_player_fire_changed(GameNetworkPlayer * game) {	
    game_notify_changed( GAME(game));
}
