/* game_1_player_manager.c
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
#include <gtk/gtk.h>
#ifdef GNOME
#include <libgnome/gnome-score.h>
#endif
#include "game-1-player-manager.h"
#include "game-1-player.h"
#include "game.h"
#include "game-manager.h"
#include "ui-main.h"

#ifdef MAEMO
#include "global.h"
#endif
#define PRIVATE(game_1_player_manager) (game_1_player_manager->private )

static GObjectClass* parent_class = NULL;

struct Game1PlayerManagerPrivate {
  MonkeyCanvas * canvas;
  GtkWidget * window;
  Game1Player * current_game;
  int current_level;
  gint current_score;
};

static void game_1_player_manager_state_changed(Game * game,Game1PlayerManager * g);

static void game_1_player_manager_start_level(Game1PlayerManager * g);

static void game_1_player_manager_game_manager_iface_init(GameManagerClass * i);

static void game_1_player_manager_finalize(GObject* object);

G_DEFINE_TYPE_WITH_CODE (Game1PlayerManager, game_1_player_manager, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (TYPE_GAME_MANAGER, game_1_player_manager_game_manager_iface_init));

static void
game_1_player_manager_init (Game1PlayerManager* game_1_player_manager)
{
	game_1_player_manager->private = g_new0 (Game1PlayerManagerPrivate, 1);
}


static void game_1_player_manager_class_init (Game1PlayerManagerClass *klass) {
  GObjectClass* object_class;

  parent_class = g_type_class_peek_parent(klass);
  object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = game_1_player_manager_finalize;
}

Game1PlayerManager * game_1_player_manager_new(GtkWidget * window,MonkeyCanvas * canvas) {
  Game1PlayerManager * game_1_player_manager;
  game_1_player_manager = GAME_1_PLAYER_MANAGER (g_object_new (TYPE_GAME_1_PLAYER_MANAGER, NULL));

  PRIVATE(game_1_player_manager)->canvas = canvas;
  PRIVATE(game_1_player_manager)->window = window;
  PRIVATE(game_1_player_manager)->current_game = NULL;
  
  return game_1_player_manager;
}


static gboolean startnew_function(gpointer data) {
  Game1PlayerManager * manager;
  UiMain * ui_main;

  manager = GAME_1_PLAYER_MANAGER(data);

  ui_main = ui_main_get_instance();

  game_stop( GAME(PRIVATE(manager)->current_game));

  g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(manager)->current_game ),
                                         G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,manager);

  g_object_unref( PRIVATE(manager)->current_game);

  ui_main_set_game(ui_main,NULL);

  if( PRIVATE(manager)->current_level < 99 ) {
  	PRIVATE(manager)->current_level++;
  } else {
	PRIVATE(manager)->current_level = 0;
  }

#ifdef MAEMO
  state.level = PRIVATE(manager)->current_level;
#endif

  game_1_player_manager_start_level(manager);
  return FALSE;
}


static gboolean restart_function(gpointer data) {
  Game1PlayerManager * manager;
  UiMain * ui_main;

  manager = GAME_1_PLAYER_MANAGER(data);

  ui_main = ui_main_get_instance();

  game_stop( GAME(PRIVATE(manager)->current_game));

  g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(manager)->current_game ),
                                         G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,manager);

  g_object_unref( PRIVATE(manager)->current_game);

  ui_main_set_game(ui_main,NULL);

  game_1_player_manager_start_level(manager);
  return FALSE;
}

static void game_1_player_manager_finalize(GObject* object) {

  Game1PlayerManager * game_1_player_manager = GAME_1_PLAYER_MANAGER(object);

  /* TODO : free current game */
  g_free(game_1_player_manager->private);

  if (G_OBJECT_CLASS (parent_class)->finalize) {
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
  }
}


static void game_1_player_manager_game_manager_iface_init(GameManagerClass * i) {
  game_manager_class_virtual_init(i,
				  game_1_player_manager_start,
				  game_1_player_manager_stop);
}

static void game_1_player_manager_state_changed(Game * game,
						Game1PlayerManager * manager) {

  UiMain * ui_main =  ui_main_get_instance();


  if( game_get_state( game ) == GAME_FINISHED ) {
    PRIVATE(manager)->current_score = game_1_player_get_score( GAME_1_PLAYER(game));
#ifdef MAEMO
    state.score = PRIVATE(manager)->current_score;
#endif
    if( game_1_player_is_lost( GAME_1_PLAYER(game) )) {
      ui_main_set_game(ui_main,NULL);	 
      g_timeout_add(2000,
		    restart_function,
		    manager);
    } else {
      ui_main_set_game(ui_main,NULL);	 
      g_timeout_add(2000,
		    startnew_function,
		    manager);
				
    }
  }
				
}

static void game_1_player_manager_start_level(Game1PlayerManager * g) {
  Game1Player * game;
  Game1PlayerManager * manager;
  UiMain * ui_main =  ui_main_get_instance();

  manager = GAME_1_PLAYER_MANAGER(g);

  ui_main_set_game(ui_main,
		   GAME( game = game_1_player_new( PRIVATE(manager)->window,
						   PRIVATE(manager)->canvas,
						   PRIVATE(manager)->current_level,
						   PRIVATE(manager)->current_score)									 
			 ));


  game_start( GAME(game) );
  
  g_signal_connect( G_OBJECT(game), "state-changed",
		    G_CALLBACK(game_1_player_manager_state_changed),manager);
  PRIVATE(manager)->current_game = game;
  monkey_canvas_paint( PRIVATE(manager)->canvas);

}

void game_1_player_manager_start(GameManager * g) {
	 
  Game1PlayerManager * manager;
	 
  manager = GAME_1_PLAYER_MANAGER(g);

#ifdef GNOME
  PRIVATE(manager)->current_level = 0;
  PRIVATE(manager)->current_score = 0;
#endif
#ifdef MAEMO
  if (state.game == 1 && state.level > 0 ) {
    PRIVATE(manager)->current_level = state.level;
    PRIVATE(manager)->current_score = state.score;
  } else {
    state.level = 0;
    PRIVATE(manager)->current_level = 0;
    PRIVATE(manager)->current_score = 0;
  }
  state.game = 1;
#endif

  game_1_player_manager_start_level(manager);

}

void game_1_player_manager_stop(GameManager * g) {
  Game1PlayerManager * manager;
  UiMain * ui_main =  ui_main_get_instance();


  manager = GAME_1_PLAYER_MANAGER(g);
  game_stop( GAME(PRIVATE(manager)->current_game));

  g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(manager)->current_game ),
                                         G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,manager);

#ifdef GNOME
  gnome_score_log(0.0 + game_1_player_get_score(PRIVATE(manager)->current_game), NULL, TRUE);
#endif

  g_object_unref( PRIVATE(manager)->current_game);

  ui_main_set_game(ui_main,NULL);
  monkey_canvas_paint( PRIVATE(manager)->canvas);
}
