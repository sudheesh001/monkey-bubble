/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* game-2-player.c
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

#include "clock.h"

#include "monkey-canvas.h"
#include "monkey-view.h"

#include "game.h"
#include "game-2-player.h"

#include "player-input.h"
#include "input-manager.h"

#define FRAME_DELAY 10
#define INIT_BUBBLES_COUNT 8+7+8+7
#define PRIVATE(game_2_player) (game_2_player->private)


static GObjectClass* parent_class = NULL;

struct Game2PlayerPrivate {
        MonkeyCanvas * canvas;
        GtkWidget * window;
        MonkeyView * display_left;
        MonkeyView * display_right;
        Block * background;
        Block * paused_block;
        Layer * paused_layer;
        Monkey * monkey_left;
        Monkey * monkey_right;
        guint timeout_id;
        GameState state;
        MbClock * clock;
        int winner;

        MbPlayerInput * p1_input;
        MbPlayerInput * p2_input;
};


static gboolean pressed(MbPlayerInput * input,
                        gint key,
                        Game2Player * game);
static gboolean released(MbPlayerInput * input,
                         gint key,
                         Game2Player * game);

static void game_2_player_fire_changed(Game2Player * game);

static void game_2_player_game_lost(Monkey * monkey,Game2Player * g);

static void game_2_player_bubbles_exploded( Monkey * monkey,
                                            GList * exploded,
                                            GList * fallen,
                                            Game2Player * g);


static void game_2_player_bubble_shot( Monkey * monkey,
                                       Bubble * bubble,
                                       Game2Player * g);

static gint game_2_player_add_bubble(Game2Player * game,Monkey * m);

static void game_2_player_bubble_sticked( Monkey * monkey,
                                          Bubble * bubble,
                                          Game2Player * g);

static void game_2_player_start(Game * game);
static void game_2_player_stop(Game * game);
static void game_2_player_pause(Game * game,gboolean pause);

static GameState game_2_player_get_state(Game * game);


static gint game_2_player_timeout (gpointer data);



static gint get_time(Game2Player * game);
static void time_paused(Game2Player * game);
static void time_init(Game2Player * game);


static void 
game_2_player_finalize(GObject* object)
{

        Game2Player * game = GAME_2_PLAYER(object);


        g_signal_handlers_disconnect_by_func( G_OBJECT(PRIVATE(game)->p1_input) ,
                                              GTK_SIGNAL_FUNC (pressed),game);


        g_signal_handlers_disconnect_by_func( G_OBJECT(PRIVATE(game)->p1_input) ,
                                              GTK_SIGNAL_FUNC (released),game);




        g_signal_handlers_disconnect_by_func( G_OBJECT(PRIVATE(game)->p2_input) ,
                                              GTK_SIGNAL_FUNC (pressed),game);


        g_signal_handlers_disconnect_by_func( G_OBJECT(PRIVATE(game)->p2_input) ,
                                              GTK_SIGNAL_FUNC (released),game);


        g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(game)->monkey_left ),
                                               G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,game);

        g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(game)->monkey_right ),
                                               G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,game);

        g_object_unref( PRIVATE(game)->clock);
        gtk_timeout_remove( PRIVATE(game)->timeout_id);

        g_object_unref( PRIVATE(game)->display_left );


        g_object_unref( PRIVATE(game)->display_right );

  
        g_object_unref( PRIVATE(game)->monkey_left);


        g_object_unref( PRIVATE(game)->monkey_right);

        monkey_canvas_unref_block(game->private->canvas, 
                                  game->private->paused_block);

        g_free(game->private);

  
        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }
}





Game2Player *
game_2_player_new(GtkWidget * window,MonkeyCanvas * canvas,int score_left,int score_right)
{
        Game2Player * game;
        Bubble * bubbles_left[INIT_BUBBLES_COUNT];
        Bubble * bubbles_right[INIT_BUBBLES_COUNT];
        Bubble * b_left;
        Bubble * b_right;
        MbInputManager * input_manager;
        gint i;
        game = GAME_2_PLAYER (g_object_new (TYPE_GAME_2_PLAYER, NULL));




        monkey_canvas_clear(canvas);
        PRIVATE(game)->clock = mb_clock_new();

        PRIVATE(game)->monkey_left = monkey_new(FALSE);  
        PRIVATE(game)->monkey_right = monkey_new(FALSE);

        for(i = 0; i < INIT_BUBBLES_COUNT; i++ ) {
                b_left = bubble_new(rand()%COLORS_COUNT,0,0);
                b_right = bubble_new( bubble_get_color(b_left),0,0);
                bubbles_left[i]=b_left;
                bubbles_right[i]= b_right;
        }


        board_init( playground_get_board( monkey_get_playground( PRIVATE(game)->monkey_left )),bubbles_left,INIT_BUBBLES_COUNT);
        board_init( playground_get_board( monkey_get_playground( PRIVATE(game)->monkey_right )),bubbles_right,INIT_BUBBLES_COUNT);


        PRIVATE(game)->display_left = 
                monkey_view_new(canvas, 
                                PRIVATE(game)->monkey_left,
                                -165,0,NULL,FALSE,TRUE);

        PRIVATE(game)->display_right = 
                monkey_view_new(canvas, 
                                PRIVATE(game)->monkey_right,
                                165,0,NULL,FALSE,TRUE);

        PRIVATE(game)->canvas = canvas;

        PRIVATE(game)->window = window;
    

        PRIVATE(game)->paused_block = 
                monkey_canvas_create_block_from_image(canvas,
                                                      DATADIR"/monkey-bubble/gfx/pause.svg",
                                                      200,200,
                                                      100,100);

        PRIVATE(game)->paused_layer =
                monkey_canvas_append_layer(canvas,0,0);

  
        PRIVATE(game)->background = 
                monkey_canvas_create_block_from_image(
                                                      canvas,
                                                      DATADIR"/monkey-bubble/gfx/layout_2_players.svg",
                                                      640,480,
                                                      0,0);
  

        monkey_canvas_add_block(canvas,
                                monkey_canvas_get_root_layer( PRIVATE(game)->canvas),
                                PRIVATE(game)->background,
                                0,0);
    
        PRIVATE(game)->timeout_id = 
                gtk_timeout_add (FRAME_DELAY, game_2_player_timeout, game);
  
  
        PRIVATE(game)->state = GAME_STOPPED;
  

  
        monkey_view_set_score( PRIVATE(game)->display_left,score_left);

        monkey_view_set_score( PRIVATE(game)->display_right,score_right);

  
        g_signal_connect( G_OBJECT( PRIVATE(game)->monkey_left),
                          "bubble-sticked",
                          G_CALLBACK(game_2_player_bubble_sticked),
                          game);

        g_signal_connect( G_OBJECT( PRIVATE(game)->monkey_left),
                          "game-lost",
                          G_CALLBACK(game_2_player_game_lost),
                          game);

        g_signal_connect( G_OBJECT( PRIVATE(game)->monkey_left),
                          "bubbles-exploded",
                          G_CALLBACK(game_2_player_bubbles_exploded),
                          game);

        g_signal_connect( G_OBJECT( PRIVATE(game)->monkey_left),
                          "bubble-shot",
                          G_CALLBACK(game_2_player_bubble_shot),
                          game);


  
        g_signal_connect( G_OBJECT( PRIVATE(game)->monkey_right),
                          "bubble-sticked",
                          G_CALLBACK(game_2_player_bubble_sticked),
                          game);

        g_signal_connect( G_OBJECT( PRIVATE(game)->monkey_right),
                          "game-lost",
                          G_CALLBACK(game_2_player_game_lost),
                          game);

        g_signal_connect( G_OBJECT( PRIVATE(game)->monkey_right),
                          "bubbles-exploded",
                          G_CALLBACK(game_2_player_bubbles_exploded),
                          game);

        g_signal_connect( G_OBJECT( PRIVATE(game)->monkey_right),
                          "bubble-shot",
                          G_CALLBACK(game_2_player_bubble_shot),
                          game);


        shooter_add_bubble(monkey_get_shooter(PRIVATE(game)->monkey_right),bubble_new( game_2_player_add_bubble(game,PRIVATE(game)->monkey_left),0,0));


        shooter_add_bubble(monkey_get_shooter(PRIVATE(game)->monkey_right),bubble_new( game_2_player_add_bubble(game,PRIVATE(game)->monkey_left),0,0));



 
        input_manager = mb_input_manager_get_instance();
        PRIVATE(game)->p1_input = mb_input_manager_get_left(input_manager);
        PRIVATE(game)->p2_input = mb_input_manager_get_right(input_manager);



        g_signal_connect(PRIVATE(game)->p1_input ,"notify-pressed",
                         GTK_SIGNAL_FUNC (pressed),game);
         
        g_signal_connect(PRIVATE(game)->p1_input ,"notify-released",
                         GTK_SIGNAL_FUNC (released),game);
         
        g_signal_connect(PRIVATE(game)->p2_input ,"notify-pressed",
                         GTK_SIGNAL_FUNC (pressed),game);
         
        g_signal_connect(PRIVATE(game)->p2_input ,"notify-released",
                         GTK_SIGNAL_FUNC (released),game);
        return game;
}


static gboolean
pressed(MbPlayerInput * input,
        gint key,
        Game2Player * game)
{
  
        Monkey * monkey;
  
	
        if( PRIVATE(game)->state == GAME_PLAYING) {

                if( input == PRIVATE(game)->p1_input) {
                        monkey = PRIVATE(game)->monkey_left;
                } else {
                        monkey = PRIVATE(game)->monkey_right;                         
                }

                if( key == LEFT_KEY ) {
                        monkey_left_changed( monkey,TRUE,get_time(game));
                } else if( key == RIGHT_KEY ) {
                        monkey_right_changed( monkey,TRUE,get_time(game));
                } else if( key == SHOOT_KEY ) {
                        monkey_shoot( monkey,get_time(game));
                }
                 

        }
         
        return FALSE;
}

static gboolean
released(MbPlayerInput * input,
         gint key,
         Game2Player * game)
{
        Monkey * monkey;

        
         
        if( PRIVATE(game)->state == GAME_PLAYING ) {
                
                
                if( input == PRIVATE(game)->p1_input) {
                        monkey = PRIVATE(game)->monkey_left;
                } else {
                        monkey = PRIVATE(game)->monkey_right;                         
                }
                
                if( key == LEFT_KEY ) {
                        monkey_left_changed( monkey,FALSE,get_time(game));
                } else if( key == RIGHT_KEY ) {
                        monkey_right_changed( monkey,FALSE,get_time(game));
                }
                
        }
        return FALSE;

}

static gint 
game_2_player_timeout (gpointer data)
{


        Game2Player * game;
        Monkey * monkey;
        gint time;

        g_assert( IS_GAME_2_PLAYER(data) );

        game = GAME_2_PLAYER(data);

        time = get_time(game);
  
        if( PRIVATE(game)->state == GAME_PLAYING ) {
  
                monkey = PRIVATE(game)->monkey_left;
    
                monkey_update( monkey,
                               time );
    

                monkey = PRIVATE(game)->monkey_right;
    
                monkey_update( monkey,
                               time );
    
    
        }

        monkey_view_update( PRIVATE(game)->display_left,
                            time);
    


        monkey_view_update( PRIVATE(game)->display_right,
                            time);
    

        monkey_canvas_paint(PRIVATE(game)->canvas);

        return TRUE;
}

static gint
get_time(Game2Player * game) 
{
        return mb_clock_get_time(PRIVATE(game)->clock);
}


static void 
time_paused(Game2Player * game) 
{
        mb_clock_pause( PRIVATE(game)->clock,TRUE);
        
}

static void 
time_unpaused(Game2Player * game) 
{
        mb_clock_pause( PRIVATE(game)->clock,FALSE);
}

static void 
time_init(Game2Player * game) 
{
        mb_clock_start( PRIVATE(game)->clock);
}

static void 
game_2_player_start(Game * game) 
{
        Game2Player * g;
        
        g_assert( IS_GAME_2_PLAYER(game));
        
        g = GAME_2_PLAYER(game);
        
        time_init( g );
        
        PRIVATE(g)->state = GAME_PLAYING;
}

static void
game_2_player_stop(Game * game) 
{
        Game2Player * g;
        
        g_assert( IS_GAME_2_PLAYER(game));
        
        g = GAME_2_PLAYER(game);
        
        PRIVATE(g)->state = GAME_STOPPED;
}


static void 
game_2_player_pause(Game * game,gboolean pause) 
{
        Game2Player * g;
        g_assert( IS_GAME_2_PLAYER(game));
  
        g = GAME_2_PLAYER(game);

        if( pause ) {
                PRIVATE(g)->state = GAME_PAUSED;
                time_paused( g);
                monkey_canvas_add_block(PRIVATE(g)->canvas,
                                        PRIVATE(g)->paused_layer,
                                        PRIVATE(g)->paused_block,
                                        320,240);
                game_2_player_fire_changed(g);


        } else {
                PRIVATE(g)->state = GAME_PLAYING;
                time_unpaused( g);
                monkey_canvas_remove_block(PRIVATE(g)->canvas,
                                           PRIVATE(g)->paused_block);    
                game_2_player_fire_changed(g);

        }
}

static GameState
game_2_player_get_state(Game * game) 
{
        Game2Player * g;
        g_assert( IS_GAME_2_PLAYER(game));
  
        g = GAME_2_PLAYER(game);

        return PRIVATE(g)->state;
}


static void 
game_2_player_game_lost(Monkey * monkey,Game2Player * g) 
{
        g_assert( IS_GAME_2_PLAYER(g));

        if( PRIVATE(g)->monkey_left == monkey ) {
                monkey_view_draw_lost(PRIVATE(g)->display_left);
                monkey_view_draw_win(PRIVATE(g)->display_right);

                PRIVATE(g)->winner = PLAYER_2;
        } else {
                PRIVATE(g)->winner = PLAYER_1;

                monkey_view_draw_win(PRIVATE(g)->display_left);
                monkey_view_draw_lost(PRIVATE(g)->display_right);

        }

        monkey_canvas_paint(PRIVATE(g)->canvas);

        time_paused(g);
  
        game_2_player_stop(GAME(g));
        PRIVATE(g)->state = GAME_FINISHED;

        game_2_player_fire_changed(g);

}

static void 
game_2_player_bubbles_exploded(Monkey * monkey,
                               GList * exploded,
                               GList * fallen,
                               Game2Player * p) 
{
        int i;
        guint8 * columns;
        int to_go;
        MonkeyView * view;
        Monkey * other;
        Color * colors;

        g_assert( IS_GAME_2_PLAYER(p));
  
        to_go = MAX(0,-3 + g_list_length(exploded)  + g_list_length(fallen)*1.5);

        if( to_go != 0) {

                colors = g_malloc( sizeof(Color)*to_go );

                for( i = 0 ; i < to_go ; i++ ) {
                        colors[i] = rand()%COLORS_COUNT;
                }


                other = PRIVATE(p)->monkey_left;
                view = PRIVATE(p)->display_left;
                if( monkey == other ) {
                        view = PRIVATE(p)->display_right;

                        other = PRIVATE(p)->monkey_right;
                }

                columns = monkey_add_bubbles( other, 
                                              to_go,
                                              colors); 
  

                g_free(columns); 
                g_free(colors); 
        }
}

static gint 
game_2_player_add_bubble(Game2Player * game,Monkey * monkey) 
{
        gint * colors_count;
        gint rnd,count;

        g_assert( IS_GAME_2_PLAYER(game));

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

        return count;

}

static void
game_2_player_bubble_shot(   Monkey * monkey,
                             Bubble * bubble,
                             Game2Player * g) {
}


static void
game_2_player_bubble_sticked(Monkey * monkey,
                             Bubble * bubble,
                             Game2Player * g) {
        
        
        Bubble ** bubbles;
        int i;
        
        g_assert( IS_GAME_2_PLAYER(g));

        
        


        if( ! monkey_is_empty( monkey) ) {
                game_2_player_add_bubble(g,monkey);
        }

 
  
        if( (monkey_get_shot_count( monkey ) % 12) == 0) {
    
                bubbles = g_malloc( sizeof(Bubble *) * 8 );
  
                for(i = 0;i < 8; i++) {
                        bubbles[i] =bubble_new(rand()%COLORS_COUNT,0,0);
                }
                monkey_insert_bubbles(monkey,bubbles);

                g_free(bubbles);
        }

}

int 
game_2_player_get_winner(Game2Player * g) 
{
        g_assert( IS_GAME_2_PLAYER(g));
        return PRIVATE(g)->winner;
}


static 
void game_2_player_fire_changed(Game2Player * game ) 
{
        game_notify_changed( GAME(game));
}


static void 
game_2_player_instance_init(Game2Player * game) 
{
        game->private =g_new0 (Game2PlayerPrivate, 2);		
}

static void
game_2_player_class_init (Game2PlayerClass *klass)
{

        GObjectClass * object_class;
        GameClass * i;

        parent_class = g_type_class_peek_parent(klass);
        object_class = G_OBJECT_CLASS(klass);
        object_class->finalize = game_2_player_finalize;

        i = &(klass->parent_class);
        i->start = game_2_player_start;
        i->stop = game_2_player_stop;
        i->pause = game_2_player_pause;
        i->get_state = game_2_player_get_state;
        
}


GType 
game_2_player_get_type(void) 
{
        static GType game_2_player_type = 0;
    
        if (!game_2_player_type) {
                static const GTypeInfo game_2_player_info = {
                        sizeof(Game2PlayerClass),
                        NULL,           /* base_init */
                        NULL,           /* base_finalize */
                        (GClassInitFunc) game_2_player_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof(Game2Player),
                        2,              /* n_preallocs */
                        (GInstanceInitFunc) game_2_player_instance_init,
                };



      
      
                game_2_player_type = g_type_register_static(TYPE_GAME,
                                                            "Game2Player",
                                                            &game_2_player_info, 0);
    
        }
        
        return game_2_player_type;
}
