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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include "game-1-player.h"
#include "game.h"
#include "monkey-view.h"
#include "clock.h"
#include "player-input.h"
#include "input-manager.h"
#include "game-sound.h"
#define FRAME_DELAY 10

#ifdef MAEMO
#include "global.h"
#endif

#define PRIVATE(game_1_player) (game_1_player->private)


static GObjectClass* parent_class = NULL;

struct Game1PlayerPrivate {
        MonkeyCanvas * canvas;
        GtkWidget * window;
        MonkeyView * display;
        Monkey * monkey;
        guint timeout_id;
        GameState state;
        MbClock * clock;
    
        gboolean lost;
    
        gint score;
        Block * paused_block;
        Layer * paused_layer;
        MbPlayerInput * input;
};

static void game_1_player_add_to_score(Game1Player * g,gint points);

static void game_1_player_bubble_sticked(Monkey * monkey,Bubble * b,Game1Player * game);

static void game_1_player_game_lost(Monkey * monkey,Game1Player * game);

static void game_1_player_bubbles_exploded(Monkey * monkey,
					   GList * exploded,
					   GList * fallen,
					   Game1Player * game);

static void game_1_player_bubble_shot(Monkey * monkey,
				      Bubble * bubble,
				      Game1Player * game);

static void game_1_player_add_bubble(Game1Player * game);

static void game_1_player_start(Game * game);
static void game_1_player_stop(Game * game);
static void game_1_player_pause(Game * game,gboolean pause);

static GameState game_1_player_get_state(Game * game);

void game_1_player_fire_changed(Game1Player * game);



static gint game_1_player_timeout (gpointer data);

static gboolean released(MbPlayerInput * input,
			 gint key,
			 Game1Player * game);


static gboolean pressed(MbPlayerInput * input,
			gint key,
			Game1Player * game);


static gint get_time(Game1Player * game);
static void time_paused(Game1Player * game);
static void time_init(Game1Player * game);

G_DEFINE_TYPE (Game1Player, game_1_player, TYPE_GAME);

#ifdef MAEMO
void game_1_player_save(Game1Player *game)
{
        //monkey_save(PRIVATE(game)->monkey, MONKEY_TEMP); //TODO: enable saving
}
#endif

static void
game_1_player_init (Game1Player* game)
{
	game->private = g_new0 (Game1PlayerPrivate, 1);
}

static void
game_1_player_finalize (GObject* object)
{
  Game1Player* game = GAME_1_PLAYER (object);

  g_source_remove (PRIVATE (game)->timeout_id);

  g_signal_handlers_disconnect_by_func (PRIVATE (game)->input, pressed, game);
  g_signal_handlers_disconnect_by_func (PRIVATE (game)->input, released, game);

  g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(game)->monkey ),
                                         G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,game);



  g_object_unref( PRIVATE(game)->clock);
  g_object_unref( PRIVATE(game)->display );


  g_object_unref( PRIVATE(game)->monkey);

  monkey_canvas_unref_block(game->private->canvas, 
                            game->private->paused_block);

  g_free(game->private);

  if (G_OBJECT_CLASS (parent_class)->finalize) {
          (* G_OBJECT_CLASS (parent_class)->finalize) (object);
  }

}


static void game_1_player_class_init (Game1PlayerClass *klass) {

        GObjectClass* object_class;
        GameClass * game_class;
        parent_class = g_type_class_peek_parent(klass);
        object_class = G_OBJECT_CLASS(klass);
        object_class->finalize = game_1_player_finalize;

        game_class = &(klass->parent_class);
        game_class->start = game_1_player_start;
        game_class->stop = game_1_player_stop;
        game_class->pause = game_1_player_pause;
        game_class->get_state = game_1_player_get_state;

}

static void game_1_player_bubble_sticked(Monkey * monkey,Bubble * b,
					 Game1Player * game) {


        g_assert( IS_GAME_1_PLAYER(game));

        if( ! monkey_is_empty( monkey) ) {                
                game_1_player_add_bubble(GAME_1_PLAYER(game));
        }
        
        if( ( monkey_get_shot_count( monkey) % 8 ) == 0 ) {

                monkey_set_board_down( monkey);
        }


}




Game1Player * game_1_player_new(GtkWidget * window,MonkeyCanvas * canvas, int level,gint score) {
        Game1Player * game;

        MbGameSound * mgs;
        MbInputManager * input_manager;
        game = GAME_1_PLAYER (g_object_new (TYPE_GAME_1_PLAYER, NULL));



        monkey_canvas_clear(canvas);
#ifdef GNOME
        PRIVATE(game)->monkey = 
                monkey_new_level_from_file(DATADIR"/monkey-bubble/levels",
                                           level);
#endif

#ifdef MAEMO
	if (state.loadmap==1) {
          PRIVATE(game)->monkey =
                monkey_new_level_from_file(MONKEY_TEMP, 0);
          state.loadmap = 0;
        } else {
          PRIVATE(game)->monkey =
                monkey_new_level_from_file(DATADIR"/monkey-bubble/levels",
                                           level);
        }
#endif
        PRIVATE(game)->display = 
                monkey_view_new(canvas, 
                                PRIVATE(game)->monkey,0,0,DATADIR"/monkey-bubble/gfx/layout_1_player.svg",TRUE,TRUE);
  
        PRIVATE(game)->canvas = canvas;
        PRIVATE(game)->window = window;
  
        monkey_view_set_score(PRIVATE(game)->display,level+1);

  
  

        PRIVATE(game)->paused_block = 
                monkey_canvas_create_block_from_image(canvas,
                                                      DATADIR"/monkey-bubble/gfx/pause.svg",
                                                      200,200,
                                                      100,100);

        PRIVATE(game)->paused_layer =
                monkey_canvas_append_layer(canvas,0,0);


        PRIVATE(game)->clock = mb_clock_new();
        PRIVATE (game)->timeout_id = g_timeout_add (FRAME_DELAY, game_1_player_timeout, game);

        PRIVATE(game)->state = GAME_STOPPED;

        PRIVATE(game)->lost = FALSE;
  
        PRIVATE(game)->score = score;

        monkey_view_set_points( PRIVATE(game)->display,score);

        g_signal_connect( G_OBJECT( PRIVATE(game)->monkey),
                          "bubble-sticked",
                          G_CALLBACK(game_1_player_bubble_sticked),
                          game);

        g_signal_connect( G_OBJECT( PRIVATE(game)->monkey),
                          "game-lost",
                          G_CALLBACK(game_1_player_game_lost),
                          game);

        g_signal_connect( G_OBJECT( PRIVATE(game)->monkey),
                          "bubbles-exploded",
                          G_CALLBACK(game_1_player_bubbles_exploded),
                          game);

        g_signal_connect( G_OBJECT( PRIVATE(game)->monkey),
                          "bubble-shot",
                          G_CALLBACK(game_1_player_bubble_shot),
                          game);


        game_1_player_add_bubble(game);

        game_1_player_add_bubble(game);

        mgs = mb_game_sound_new();
        mb_game_sound_connect_monkey(mgs,PRIVATE(game)->monkey);
        input_manager = mb_input_manager_get_instance();
        PRIVATE(game)->input = mb_input_manager_get_left(input_manager);

        g_signal_connect (PRIVATE (game)->input ,"notify-pressed",
                          G_CALLBACK (pressed),game);
        g_signal_connect (PRIVATE (game)->input ,"notify-released",
                          G_CALLBACK (released),game);

        return game;
}






static gboolean
pressed(MbPlayerInput * input,
	gint key,
	Game1Player * game)
{  
 
        Monkey * monkey;
  

        if( PRIVATE(game)->state == GAME_PLAYING) {
	
                monkey = PRIVATE(game)->monkey;
    
                if( key == LEFT_KEY) {
                        monkey_left_changed( monkey,TRUE,get_time(game));
                } else if ( key == RIGHT_KEY) {
                        monkey_right_changed( monkey,TRUE,get_time(game));
                } else if( key == SHOOT_KEY) {
                        monkey_shoot( monkey,get_time(game));
                }
        }

        return FALSE;
}


static gboolean 
released(MbPlayerInput * input,
	 gint key,
	 Game1Player * game)
{
        Monkey * monkey;

        monkey = PRIVATE(game)->monkey;

        if( PRIVATE(game)->state == GAME_PLAYING ) {

                if( key == LEFT_KEY) {
                        monkey_left_changed( monkey,FALSE,get_time(game));
                }
    
                if( key == RIGHT_KEY) {
                        monkey_right_changed( monkey,FALSE,get_time(game));
                }
      
        }
        return FALSE;

}

static gint game_1_player_timeout (gpointer data)
{


        Game1Player * game;
        Monkey * monkey;
        gint time;

        game = GAME_1_PLAYER(data);
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

static gint get_time(Game1Player * game) {
        return mb_clock_get_time(PRIVATE(game)->clock);
}


static void time_paused(Game1Player * game) {
        mb_clock_pause( PRIVATE(game)->clock, TRUE);
}

static void time_unpaused(Game1Player * game) {
        mb_clock_pause( PRIVATE(game)->clock, FALSE);
}

static void time_init(Game1Player * game) {
        mb_clock_start( PRIVATE(game)->clock);
}

static void game_1_player_start(Game * game) {
        Game1Player * g;
 
        g_assert( IS_GAME_1_PLAYER(game));
  
        g = GAME_1_PLAYER(game);

        time_init( g );

        PRIVATE(g)->state = GAME_PLAYING;
}

static void game_1_player_stop(Game * game) {
        Game1Player * g;

        g_assert( IS_GAME_1_PLAYER(game));
  
        g = GAME_1_PLAYER(game);

        PRIVATE(g)->state = GAME_STOPPED;

        //  time_paused(g);
        game_1_player_fire_changed(g);

}

gboolean game_1_player_is_lost(Game1Player * g) {

        g_assert( GAME_1_PLAYER(g));

        return PRIVATE(g)->lost;
}


gint game_1_player_get_score(Game1Player * g) {

        g_assert( GAME_1_PLAYER(g));

        return PRIVATE(g)->score;
}

static void game_1_player_pause(Game * game,gboolean pause) {
        Game1Player * g;
        g_assert( IS_GAME_1_PLAYER(game));
  
        g = GAME_1_PLAYER(game);

        if( pause ) {
                PRIVATE(g)->state = GAME_PAUSED;
                time_paused( g);

                monkey_canvas_add_block(PRIVATE(g)->canvas,
                                        PRIVATE(g)->paused_layer,
                                        PRIVATE(g)->paused_block,
                                        320,240);

                game_1_player_fire_changed(g);
        } else {
                PRIVATE(g)->state = GAME_PLAYING;
                time_unpaused( g);
                monkey_canvas_remove_block(PRIVATE(g)->canvas,
                                           PRIVATE(g)->paused_block);    

                game_1_player_fire_changed(g);

        }
}

static GameState game_1_player_get_state(Game * game) {
        Game1Player * g;
        g_assert( IS_GAME_1_PLAYER(game));
  
        g = GAME_1_PLAYER(game);

        return PRIVATE(g)->state;
}

static void game_1_player_game_lost(Monkey * monkey,Game1Player * g) {
        g_assert( IS_GAME_1_PLAYER(g));


        PRIVATE(g)->lost = TRUE;

        monkey_view_draw_lost( PRIVATE(g)->display );

        game_1_player_stop( GAME(g));


        PRIVATE(g)->state = GAME_FINISHED;
  
        game_1_player_fire_changed(g);
        monkey_canvas_paint(PRIVATE(g)->canvas);
    
}

static void game_1_player_bubbles_exploded(  Monkey * monkey,
					     GList * exploded,
					     GList * fallen,
					     Game1Player * g) {

        gint points;

        g_assert( IS_GAME_1_PLAYER(g));	 


        /**
         * evaluate score :
         * a exploded bubble = 10 pts
         * a fall bubble = 20 pts
         * a minimum of 30 pts to add to score
         */

        points = g_list_length(exploded) + g_list_length(fallen)*2;
        if( points > 3 ) {
                game_1_player_add_to_score(g,points*10);
        }    
  
        if( monkey_is_empty( monkey) ) {
    
                PRIVATE(g)->state = GAME_FINISHED;
    
                monkey_view_draw_win( PRIVATE(g)->display );
    
                game_1_player_fire_changed(g);	
                game_1_player_stop(GAME(g));
        }

  
}

static void game_1_player_add_to_score(Game1Player * g,gint points) {
        g_assert( IS_GAME_1_PLAYER(g));

        PRIVATE(g)->score += points;
        monkey_view_set_points(PRIVATE(g)->display,PRIVATE(g)->score);  
}

static void game_1_player_add_bubble(Game1Player * game) {
        gint * colors_count;
        gint rnd,count;
        Monkey * monkey;

        g_assert( IS_GAME_1_PLAYER(game));

        monkey = PRIVATE(game)->monkey;
        colors_count = board_get_colors_count(playground_get_board(monkey_get_playground(monkey)));

        rnd = rand()%COLORS_COUNT;
        count = 0;
        while( rnd >= 0 ) {
                count++;
                count %= COLORS_COUNT;

                while(colors_count[count] == 0) {
                        count++;
                        count %= COLORS_COUNT;
                }
                rnd--;
        }
  
        shooter_add_bubble(monkey_get_shooter(monkey),bubble_new(count,0,0));

}
static void game_1_player_bubble_shot( Monkey * monkey,
                                       Bubble * bubble,
				       Game1Player * game) {


        g_assert( IS_GAME_1_PLAYER(game));

}

void game_1_player_fire_changed(Game1Player * game) {	
        game_notify_changed( GAME(game));
}

/* vim:set et sw=2 cino=t0,f0,(0,{s,>2s,n-1s,^-1s,e2s: */
