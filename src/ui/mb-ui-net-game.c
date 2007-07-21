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

#include "mb-ui-net-game.h"

#include <glib.h>
#include <glib-object.h>

#include "game.h"
#include "monkey-view.h"
#include "mini-view.h"
#include "clock.h"

#include "ui-main.h"

#include "player-input.h"
#include "input-manager.h"

#include "game-sound.h"

#define FRAME_DELAY 10
typedef struct _Private {
	int i;
	GameState state;
	MonkeyCanvas *canvas;
	MbNetClientGame *game;
	MbNetClientMatch *match;

	MonkeyView *display;
	guint timeout_id;

	gboolean lost;

	gint score;
	Block *paused_block;
	Layer *paused_layer;

	guint last_shoot;
	MbMiniView *mini_views[4];

	MbPlayerInput *input;
	gboolean canShoot;
	gboolean bubbleSticked;
	guint last_sticked;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_ui_net_game_get_property(GObject * object,
					guint prop_id,
					GValue * value,
					GParamSpec * param_spec);
static void mb_ui_net_game_set_property(GObject * object,
					guint prop_id,
					const GValue * value,
					GParamSpec * param_spec);


static void _start(Game * game);
static void _stop(Game * game);
static void _pause(Game * game, gboolean pause);
static GameState _get_state(Game * game);

static gboolean _pressed(MbPlayerInput * i, gint key, MbUiNetGame * self);
static gboolean _released(MbPlayerInput * i, gint key, MbUiNetGame * self);
static gint _timeout(MbUiNetGame * self);
static gint _get_time(MbUiNetGame * self);
//static        guint   _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbUiNetGame, mb_ui_net_game, TYPE_GAME, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_UI_TYPE_NET_GAME, Private))




static void mb_ui_net_game_finalize(MbUiNetGame * self);

static void mb_ui_net_game_init(MbUiNetGame * self);



static void mb_ui_net_game_init(MbUiNetGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	UiMain *ui_main = ui_main_get_instance();
	priv->canvas = ui_main_get_canvas(ui_main);
}


static void mb_ui_net_game_finalize(MbUiNetGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}

static void _winlost(MbNetClientMatch * match, gboolean win,
		     MbUiNetGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	mb_net_client_match_lock(priv->match);
	priv->state = GAME_FINISHED;
	g_print(" winlost ... \n");
	if (win == FALSE) {
		monkey_view_draw_lost(priv->display);

	} else {
		monkey_view_draw_win(priv->display);


	}
	game_notify_changed(GAME(self));

	mb_net_client_match_unlock(priv->match);
}

static void _player_changed(MbNetClientMatch * match, guint32 player,
			    MbUiNetGame * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	Color *color = NULL;
	gboolean odd;
	guint32 count;
	g_print("player changed %d\n", player);
	mb_net_client_match_get_player_bubbles(match, player, &color,
					       &count, &odd);
	//if( monkey_id >= 1 && monkey_id <= 4 ) {
	mb_mini_view_update(priv->mini_views[player], color, odd);
	//     }
}

MbUiNetGame *mb_ui_net_game_new(MbNetClientGame * game,
				MbNetClientMatch * match)
{
	MbUiNetGame *self;
	self = MB_UI_NET_GAME(g_object_new(MB_UI_TYPE_NET_GAME, NULL));
	Private *priv;
	priv = GET_PRIVATE(self);
	g_object_ref(game);
	g_object_ref(match);

	priv->match = match;
	priv->game = game;
	MonkeyCanvas *canvas;
	canvas = priv->canvas;

	monkey_canvas_clear(canvas);
	priv->display =
	    monkey_view_new(canvas,
			    mb_net_client_match_get_monkey(priv->match),
			    -175, 0,
			    DATADIR
			    "/monkey-bubble/gfx/layout_network_player.svg",
			    TRUE, FALSE);
	priv->paused_block =
	    monkey_canvas_create_block_from_image(canvas,
						  DATADIR
						  "/monkey-bubble/gfx/pause.svg",
						  200, 200, 100, 100);

	priv->paused_layer = monkey_canvas_append_layer(canvas, 0, 0);


	monkey_view_set_score(priv->display, 0);




	priv->timeout_id =
	    gtk_timeout_add(FRAME_DELAY, (GSourceFunc) _timeout, self);


	priv->state = GAME_STOPPED;

	priv->lost = FALSE;
	monkey_view_set_points(priv->display, 0);

	/*
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


	 */
	g_signal_connect(priv->match, "winlost", (GCallback) _winlost,
			 self);
	MbGameSound *mgs = mb_game_sound_new();
	mb_game_sound_connect_monkey(mgs,
				     mb_net_client_match_get_monkey(priv->
								    match));

	MbInputManager *input_manager;
	input_manager = mb_input_manager_get_instance();
	priv->input = mb_input_manager_get_left(input_manager);


	int x = 350;
	int y = 30;
	int i;
	for (i = 0; i < 4; i++) {

		if (i == 1) {
			x += 155;
		}
		if (i == 2) {
			x -= 155;
			y += 210;
		}

		if (i == 3) {
			x += 155;
		}

		priv->mini_views[i] = mb_mini_view_new(canvas, x, y);


	}

	g_signal_connect(priv->match, "player-changed",
			 (GCallback) _player_changed, self);

	return self;

}

static gint _timeout(MbUiNetGame * self)
{


	Private *priv;
	priv = GET_PRIVATE(self);

//      Monkey *monkey;         
//      g_print("paint ... \n");
	mb_net_client_match_lock(priv->match);

	monkey_view_update(priv->display, _get_time(self));

	monkey_canvas_paint(priv->canvas);

	mb_net_client_match_unlock(priv->match);
	//g_print("painted ... \n");
	return TRUE;
}

static _match_start(MbNetClientMatch * client, MbUiNetGame * self)
{
	g_print("match started ... launch clock and display \n");
	Private *priv;
	priv = GET_PRIVATE(self);

	priv->state = GAME_PLAYING;
	game_notify_changed(GAME(self));
	g_signal_connect(priv->input, "notify-pressed",
			 GTK_SIGNAL_FUNC(_pressed), self);

	g_signal_connect(priv->input, "notify-released",
			 GTK_SIGNAL_FUNC(_released), self);

}


static gint _get_time(MbUiNetGame * self)
{
	return mb_net_client_match_get_time(GET_PRIVATE(self)->match);

}

static gboolean _pressed(MbPlayerInput * i, gint key, MbUiNetGame * self)
{

	Private *priv;
	priv = GET_PRIVATE(self);

	Monkey *monkey;


	if (priv->state == GAME_PLAYING) {
		monkey = mb_net_client_match_get_monkey(priv->match);
		mb_net_client_match_lock(priv->match);

		if (key == LEFT_KEY) {
			monkey_left_changed(monkey, TRUE, _get_time(self));
		} else if (key == RIGHT_KEY) {
			monkey_right_changed(monkey, TRUE,
					     _get_time(self));
		}
		mb_net_client_match_unlock(priv->match);
		if (key == SHOOT_KEY) {
			g_print("shoot ... \n");
			mb_net_client_match_shoot(priv->match);

		}


	}

	return FALSE;
}


static gboolean _released(MbPlayerInput * i, gint key, MbUiNetGame * self)
{

	Private *priv;
	priv = GET_PRIVATE(self);

	Monkey *monkey;


	if (priv->state == GAME_PLAYING) {
		monkey = mb_net_client_match_get_monkey(priv->match);
		mb_net_client_match_lock(priv->match);
		if (key == LEFT_KEY) {
			monkey_left_changed(monkey, FALSE,
					    _get_time(self));
		} else if (key == RIGHT_KEY) {
			monkey_right_changed(monkey, FALSE,
					     _get_time(self));
		}
		mb_net_client_match_unlock(priv->match);

	}

	return FALSE;
}


static void _start(Game * game)
{
	MbUiNetGame *self = MB_UI_NET_GAME(game);
	Private *priv;
	priv = GET_PRIVATE(self);

	g_signal_connect(priv->match, "start", (GCallback) _match_start,
			 self);
	mb_net_client_match_ready(priv->match);
}

static void _stop(Game * game)
{
}

static void _pause(Game * game, gboolean pause)
{
}

static GameState _get_state(Game * game)
{
	MbUiNetGame *self = MB_UI_NET_GAME(game);
	Private *priv;
	priv = GET_PRIVATE(self);
	return priv->state;
}


static void
mb_ui_net_game_get_property(GObject * object, guint prop_id,
			    GValue * value, GParamSpec * param_spec)
{
	MbUiNetGame *self;

	self = MB_UI_NET_GAME(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_ui_net_game_set_property(GObject * object, guint prop_id,
			    const GValue * value, GParamSpec * param_spec)
{
	MbUiNetGame *self;

	self = MB_UI_NET_GAME(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_ui_net_game_class_init(MbUiNetGameClass * mb_ui_net_game_class)
{
	GObjectClass *g_object_class;

	parent_class = g_type_class_peek_parent(mb_ui_net_game_class);


	g_type_class_add_private(mb_ui_net_game_class, sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_ui_net_game_class);

	/* setting up property system */
	g_object_class->set_property = mb_ui_net_game_set_property;
	g_object_class->get_property = mb_ui_net_game_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_ui_net_game_finalize;


	GameClass *game_class;

	game_class = &(mb_ui_net_game_class->base_class);
	game_class->start = _start;
	game_class->stop = _stop;
	game_class->pause = _pause;
	game_class->get_state = _get_state;
}
