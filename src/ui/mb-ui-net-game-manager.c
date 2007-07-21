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
typedef struct _Private {
	MonkeyCanvas *canvas;
	gboolean playing;
	MbNetClientGame *game;
	MbNetClientServer *client;
	MbNetClientMatch *match;
	MbNetServer *server;

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




static void mb_ui_net_game_manager_finalize(MbUiNetGameManager * self);

static void mb_ui_net_game_manager_init(MbUiNetGameManager * self);



static void mb_ui_net_game_manager_init(MbUiNetGameManager * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	UiMain *ui_main = ui_main_get_instance();
	priv->canvas = ui_main_get_canvas(ui_main);
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
}

void mb_ui_net_game_manager_set_client(MbUiNetGameManager * self,
				       MbNetClientServer * client)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->client = client;
	g_object_ref(client);
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

	monkey_canvas_clear(priv->canvas);
	monkey_canvas_paint(priv->canvas);

	MbUiNetGame *g = mb_ui_net_game_new(priv->game, priv->match);

	UiMain *ui_main = ui_main_get_instance();

	priv->playing = TRUE;


	ui_main_set_game(ui_main, GAME(g));


	monkey_canvas_paint(priv->canvas);

	game_start(GAME(g));
	//   monkey_canvas_paint( priv->canvas);
}

static gboolean _game_rstart_idle(MbUiNetGameManager * self)
{
	_game_rstart(self);
}

void _game_start(MbNetClientGame * game, MbNetClientMatch * match,
		 MbUiNetGameManager * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
//      g_object_ref(match);
	g_object_ref(match);
	priv->match = match;

	g_idle_add((GSourceFunc) _game_rstart_idle, self);

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

	/*

	   PRIVATE(manager)->current_level = 0;
	   PRIVATE(manager)->current_score = 0;


	   g_signal_connect( G_OBJECT(PRIVATE(manager)->handler),
	   "recv-xml-message",
	   G_CALLBACK( recv_xml_message ),
	   manager);

	   g_signal_connect( G_OBJECT( PRIVATE(manager)->handler), "recv-bubble-array",
	   G_CALLBACK(recv_bubble_array),manager);

	   PRIVATE(manager)->add_bubble_handler_id = 
	   g_signal_connect( G_OBJECT( PRIVATE(manager)->handler), "recv-add-bubble",
	   G_CALLBACK(recv_add_bubble),manager);

	   g_signal_connect( G_OBJECT( PRIVATE(manager)->handler), "recv-start",
	   G_CALLBACK(recv_start),manager);
	 */

}

void _manager_stop(GameManager * g)
{
	MbUiNetGameManager *self;
	UiMain *ui_main = ui_main_get_instance();

	self = MB_UI_NET_GAME_MANAGER(g);

	//game_stop( GAME(PRIVATE(manager)->current_game));
/*
    g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(manager)->current_game ),
					   G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,manager);
*/
//    g_object_unref( PRIVATE(manager)->current_game);
	//  PRIVATE(manager)->current_game = NULL;
	ui_main_set_game(ui_main, NULL);
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
