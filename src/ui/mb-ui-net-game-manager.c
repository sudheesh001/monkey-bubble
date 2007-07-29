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

#include "mb-ui-net-game-manager.h"

#include <glib.h>
#include <glib-object.h>
#include "game-manager.h"
#include "ui-main.h"
#include <ui/mb-ui-net-game.h>

#include <input/player-input.h>
#include <input/input-manager.h>
typedef struct _Private {
	MonkeyCanvas *canvas;
	gboolean playing;
	MbNetClientGame *game;
	MbNetClientServer *client;
	MbNetClientMatch *match;
	MbNetServer *server;
	MbUiNetGame *current_game;
	gboolean waiting_start;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_ui_net_game_manager_get_property(GObject * object,
						guint prop_id,
						GValue * value,
						GParamSpec * param_spec);
static void mb_ui_net_game_manager_set_property(GObject * object,
						guint prop_id,
						const GValue * value,
						GParamSpec * param_spec);


static void game_manager_iface_init(GameManagerClass * i);


//static        guint   _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbUiNetGameManager, mb_ui_net_game_manager,
			G_TYPE_OBJECT, {
			G_IMPLEMENT_INTERFACE(TYPE_GAME_MANAGER,
					      game_manager_iface_init)});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_UI_TYPE_NET_GAME_MANAGER, Private))



void _manager_stop(GameManager * g);
static void mb_ui_net_game_manager_finalize(MbUiNetGameManager * self);

static void mb_ui_net_game_manager_init(MbUiNetGameManager * self);




static gboolean _pressed(MbPlayerInput * i, gint key,
			 MbUiNetGameManager * self)
{

	Private *priv;
	priv = GET_PRIVATE(self);
	if (priv->waiting_start == TRUE) {

		if (key == SHOOT_KEY) {
			priv->waiting_start = FALSE;
			mb_net_client_game_start(priv->game);

		}


	}

	return FALSE;

}

static void mb_ui_net_game_manager_init(MbUiNetGameManager * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	UiMain *ui_main = ui_main_get_instance();
	priv->canvas = ui_main_get_canvas(ui_main);
	MbInputManager *input_manager;
	input_manager = mb_input_manager_get_instance();
	MbPlayerInput *input = mb_input_manager_get_left(input_manager);
	g_signal_connect(input, "notify-pressed",
			 GTK_SIGNAL_FUNC(_pressed), self);


}

static void _stop(MbNetClientGame * game, MbUiNetGameManager * self)
{
	g_print("manager game stop ... \n");
	UiMain *ui_main = ui_main_get_instance();

	ui_main_stop_game(ui_main);

}


static void mb_ui_net_game_manager_finalize(MbUiNetGameManager * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}

void mb_ui_net_game_manager_set_game(MbUiNetGameManager * self,
				     MbNetClientGame * game)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->game = game;
	g_object_ref(game);
	g_signal_connect(priv->game, "stop", (GCallback) _stop, self);

}

static void _disconnected(MbNetClientServer * client,
			  MbUiNetGameManager * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_object_unref(priv->client);
	priv->client = NULL;

	g_object_unref(priv->game);
	priv->game = NULL;

	UiMain *ui_main = ui_main_get_instance();

	ui_main_stop_game(ui_main);
}

void mb_ui_net_game_manager_set_client(MbUiNetGameManager * self,
				       MbNetClientServer * client)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->client = client;
	g_object_ref(client);
	g_signal_connect(priv->client, "disconnected",
			 (GCallback) _disconnected, self);
}

void mb_ui_net_game_manager_set_server(MbUiNetGameManager * self,
				       MbNetServer * server)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->server = server;
	g_object_ref(server);
}


static void _game_rstart(MbUiNetGameManager * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	//monkey_canvas_clear(priv->canvas);
	//monkey_canvas_paint(priv->canvas);

	MbUiNetGame *g = mb_ui_net_game_new(priv->game, priv->match);

	UiMain *ui_main = ui_main_get_instance();

	priv->playing = TRUE;


	ui_main_set_game(ui_main, GAME(g));

	priv->current_game = g;

	//monkey_canvas_paint(priv->canvas);

	game_start(GAME(g));
	//   monkey_canvas_paint( priv->canvas);
}

static gboolean _game_rstart_idle(MbUiNetGameManager * self)
{
	_game_rstart(self);
}

void _match_stopped(MbNetClientMatch * match, MbUiNetGameManager * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	if (mb_net_client_game_is_master(priv->game)) {
		priv->waiting_start = TRUE;
	}
}

void _game_start(MbNetClientGame * game, MbNetClientMatch * match,
		 MbUiNetGameManager * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
//      g_object_ref(match);
	g_object_ref(match);
	priv->match = match;

	_game_rstart(self);
	//g_idle_add((GSourceFunc) _game_rstart_idle, self);
	g_signal_connect(match, "stop", (GCallback) _match_stopped, self);
//    game_network_player_set_start_time(game,PRIVATE(manager)->start_time);

	/* g_signal_connect( G_OBJECT(game), "state-changed",
	   G_CALLBACK(state_changed),manager);
	   PRIVATE(manager)->current_game = game; */
}

void _manager_start(GameManager * g)
{

	MbUiNetGameManager *self;
	UiMain *ui_main = ui_main_get_instance();

	self = MB_UI_NET_GAME_MANAGER(g);
	Private *priv;
	priv = GET_PRIVATE(self);

	g_signal_connect(priv->game, "start",
			 (GCallback) _game_start, self);


}

void _manager_stop(GameManager * g)
{
	MbUiNetGameManager *self;
	UiMain *ui_main = ui_main_get_instance();

	self = MB_UI_NET_GAME_MANAGER(g);
	Private *priv;
	priv = GET_PRIVATE(self);

	g_print("disconnect game \n");
	ui_main_set_game(ui_main, NULL);
	g_print("disconnect ok game \n");

	g_print("free match \n");
	if (priv->match != NULL) {
		g_signal_handlers_disconnect_by_func(priv->match,
						     _match_stopped, self);

		mb_net_client_match_stop(priv->match);
		g_object_unref(priv->match);
		priv->match = NULL;
	}

	g_print("free game \n");
	if (priv->game != NULL) {
		g_signal_handlers_disconnect_by_func(priv->game,
						     _stop, self);

		mb_net_client_game_stop(priv->game);
		g_object_unref(priv->game);
		priv->game = NULL;
	}

	g_print("free client \n");
	if (priv->client != NULL) {
		g_signal_handlers_disconnect_by_func(priv->client,
						     _disconnected, self);
		mb_net_client_server_disconnect(priv->client);
		g_object_unref(priv->client);
	}

	g_print("free servver \n");
	if (priv->server != NULL) {
		mb_net_server_stop(priv->server);
		g_object_unref(priv->server);
		priv->server = NULL;
	}

	g_print("stoppp game ..... !!! \n");
	if (priv->current_game != NULL) {
		game_stop(GAME(priv->current_game));
	}
	MbInputManager *input_manager;
	input_manager = mb_input_manager_get_instance();
	MbPlayerInput *input = mb_input_manager_get_left(input_manager);


	g_signal_handlers_disconnect_matched(input,
					     G_SIGNAL_MATCH_DATA, 0, 0,
					     NULL, NULL, self);



//    g_object_unref( PRIVATE(manager)->current_game);
	//  PRIVATE(manager)->current_game = NULL;
}


static void
mb_ui_net_game_manager_get_property(GObject * object, guint prop_id,
				    GValue * value,
				    GParamSpec * param_spec)
{
	MbUiNetGameManager *self;

	self = MB_UI_NET_GAME_MANAGER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_ui_net_game_manager_set_property(GObject * object, guint prop_id,
				    const GValue * value,
				    GParamSpec * param_spec)
{
	MbUiNetGameManager *self;

	self = MB_UI_NET_GAME_MANAGER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void game_manager_iface_init(GameManagerClass * i)
{
	game_manager_class_virtual_init(i, _manager_start, _manager_stop);
}

static void
mb_ui_net_game_manager_class_init(MbUiNetGameManagerClass *
				  mb_ui_net_game_manager_class)
{
	GObjectClass *g_object_class;

	parent_class =
	    g_type_class_peek_parent(mb_ui_net_game_manager_class);


	g_type_class_add_private(mb_ui_net_game_manager_class,
				 sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_ui_net_game_manager_class);

	/* setting up property system */
	g_object_class->set_property = mb_ui_net_game_manager_set_property;
	g_object_class->get_property = mb_ui_net_game_manager_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_ui_net_game_manager_finalize;


}
