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

#include "game.h"
#include "clock.h"
#include "monkey.h"
#include "client.h"

#define INIT_BUBBLES_COUNT 8+7+8+7

#define PRIVATE(self) (self->private )
static GObjectClass* parent_class = NULL;


enum {
        GAME_STOPPED,
        LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL];



struct NetworkGamePrivate 
{
	GList * clients;
	GMutex * clients_lock;
	Clock * clock;
};


struct Client {
        Monkey * monkey;
	NetworkClient * client;
	NetworkGame * game;
        GMutex * monkey_lock;
	guint disconnected_handler_id;
};


static void init_clients(NetworkGame * self);
static void client_disconnected(NetworkMessageHandler * handler,
				struct Client * client);

static void 
client_disconnected(NetworkMessageHandler * handler,
		    struct Client * client)
{

	NetworkGame * self;

	self = client->game;

	g_mutex_lock(PRIVATE(self)->clients_lock);
	PRIVATE(self)->clients =
		g_list_remove( PRIVATE(self)->clients,
			       client);

	g_object_unref( client->monkey);


	g_signal_handler_disconnect( client->client, client->disconnected_handler_id);

	g_mutex_free( client->monkey_lock);
	g_object_unref( client->client);

	g_free( client);

	g_mutex_unlock(PRIVATE(self)->clients_lock);

}

static void 
add_client(gpointer data,gpointer user_data) 
{

	NetworkClient * client;
	NetworkGame * game;
	struct Client * c;

	client = NETWORK_CLIENT(data);
	game = NETWORK_GAME(user_data);

	c = g_malloc( sizeof(struct Client));


	c->client = client;
	
	g_object_ref( G_OBJECT(client));

	c->game = game;

	c->monkey_lock = g_mutex_new();

	c->monkey = monkey_new(TRUE);


	c->disconnected_handler_id = g_signal_connect( G_OBJECT(client), "disconnected",
						     G_CALLBACK(client_disconnected),c);
	PRIVATE(game)->clients = g_list_append( PRIVATE(game)->clients,
						c);


	
}

static void
init_client(NetworkGame * self,struct Client * client,
	    Color * colors,
	    int count) 
{
	Monkey * m;
	Shooter * s;
	
        Bubble  ** bubbles;
        int i;
	//        Color c;

	m = client->monkey;
	
        bubbles = g_malloc( sizeof(Bubble *) * count);
	
	
	for(i = 0; i < count; i++ ) {
		bubbles[i] = bubble_new(colors[i],0,0);
	}
	
        
        board_init( playground_get_board( monkey_get_playground( m )),
                    bubbles,count);
        
        network_message_handler_send_bubble_array(network_client_get_handler(client->client),
                                                 network_client_get_id( client->client),
                                                 INIT_BUBBLES_COUNT,
                                                 colors);
	
        s = monkey_get_shooter(m);

	/*
	
        g_signal_connect( G_OBJECT( s),
                          "bubble-added",
                          G_CALLBACK(bubble_added),
                          client);
	
	
	
        g_signal_connect( G_OBJECT( m),
                          "bubble-sticked",
                          G_CALLBACK(bubble_sticked),
                          client);

        g_signal_connect( G_OBJECT(m),
                          "game-lost",
                          G_CALLBACK(game_lost),
                          client);
        
        g_signal_connect( G_OBJECT( m),
                          "bubbles-exploded",
                          G_CALLBACK(bubbles_exploded),
                          client);
	
        g_signal_connect( G_OBJECT( m),
                          "bubble-shot",
                          G_CALLBACK(bubble_shot),
                          client);

			  
        c = what_bubble( m );
        shooter_add_bubble(s,bubble_new(c,0,0));

        c = what_bubble( m );
        shooter_add_bubble(s,bubble_new(c,0,0));

        g_signal_connect( G_OBJECT(client->handler),
                          "recv-shoot",
                          G_CALLBACK( recv_shoot ),
                          client);
        
	*/
}


static void
init_clients(NetworkGame * self) 
{

	GList * next;
        Bubble  * bubbles[INIT_BUBBLES_COUNT];
        Color  colors[INIT_BUBBLES_COUNT];
        int i;
	
	for(i = 0; i < INIT_BUBBLES_COUNT; i++ ) {
		Color c = rand()%COLORS_COUNT;
		colors[i] = c;
		bubbles[i] = bubble_new(c,0,0);
	}

	next = PRIVATE(self)->clients;

	while( next != NULL) {
		struct Client * client;
		
		client = (struct Client *) next->data;
			
		init_client(self,client,colors,INIT_BUBBLES_COUNT);
		next = g_list_next(next);
	}
}

NetworkGame *
network_game_new  (GList * clients)
{
	NetworkGame * self;

	self = NETWORK_GAME( g_object_new(NETWORK_TYPE_GAME,NULL) );

	g_list_foreach( clients, add_client, self);

	init_clients(self);
	return self;
}

static void 
network_game_instance_init(NetworkGame * self) 
{
	PRIVATE(self) = g_new0 (NetworkGamePrivate, 1);			
	PRIVATE(self)->clients = NULL;
	PRIVATE(self)->clients_lock = g_mutex_new();
	PRIVATE(self)->clock = clock_new();
}


static void
network_game_finalize (GObject * object) 
{
	NetworkGame * self;

	self = NETWORK_GAME(object);
	
	g_assert( PRIVATE(self)->clients == NULL);
	
	g_object_unref( PRIVATE(self)->clock);
	g_mutex_free( PRIVATE(self)->clients_lock );
	g_free(PRIVATE(self));
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}

}


static void
network_game_class_init (NetworkGameClass	* network_game_class)
{
	GObjectClass	* g_object_class;

	parent_class = g_type_class_peek_parent(network_game_class);

	g_object_class = G_OBJECT_CLASS(network_game_class);
	g_object_class->finalize = network_game_finalize;

        signals[GAME_STOPPED]= g_signal_new ("game-stopped",
                                             G_TYPE_FROM_CLASS (network_game_class),
                                             G_SIGNAL_RUN_FIRST |
                                             G_SIGNAL_NO_RECURSE,
                                             G_STRUCT_OFFSET (NetworkGameClass, game_stopped),
                                             NULL, NULL,
                                             g_cclosure_marshal_VOID__VOID,
                                             G_TYPE_NONE,

                                             0,NULL);

}


GType
network_game_get_type (void)
{
	static GType	type = 0;

	if (!type)
	{
		const GTypeInfo info = {
			sizeof (NetworkGameClass),
			NULL,	/* base initializer */
			NULL,	/* base finalizer */
			(GClassInitFunc)network_game_class_init,
			NULL,	/* class finalizer */
			NULL,	/* class data */
			sizeof (NetworkGame),
			1,
			(GInstanceInitFunc) network_game_instance_init,
			0
		};

		type = g_type_register_static (
				G_TYPE_OBJECT,
				"NetworkGame",
				&info,
				0);
	}

	return type;
}

