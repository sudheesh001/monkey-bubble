/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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

#include "client.h"

#define PRIVATE(self) (self->private )
static GObjectClass* parent_class = NULL;


enum {
	STATE_CHANGED,
	DISCONNECT_REQUEST,
	START_REQUEST,
	DISCONNECTED,
	GAME_CREATED_OK,
	NUMBER_OF_GAMES,
	NUMBER_OF_PLAYERS,
	LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL];


struct NetworkClientPrivate 
{
	NetworkPlayer * player;
	NetworkMessageHandler * handler;
	guint32 client_id;
	gboolean state;
};

static void  recv_xml_message(NetworkMessageHandler * mmh,
			      guint32 client_id,
			      xmlDoc * doc,
			      NetworkClient * client);

static void client_connection_closed(NetworkMessageHandler * mmh,
				     NetworkClient * client);

NetworkClient *
network_client_new  (guint32 client_id)
{
	NetworkClient * self;

	self = NETWORK_CLIENT( g_object_new(NETWORK_TYPE_CLIENT,NULL) );
	self->id = client_id;
	return self;
}

void network_client_set_player(NetworkClient * client,
			       NetworkPlayer * player) {

	g_assert(NETWORK_IS_CLIENT(client));
	g_assert(PRIVATE(client)->player == NULL);

	PRIVATE(client)->player = player;
	g_object_ref( G_OBJECT(player));
}



NetworkPlayer * network_client_get_player(NetworkClient * client) 
{
	g_assert(NETWORK_IS_CLIENT(client));

	return PRIVATE(client)->player;
}


gboolean network_client_get_state(NetworkClient * client) {
	return PRIVATE(client)->state;
}

void network_client_set_handler(NetworkClient * client,
				NetworkMessageHandler * handler) 
{
	g_assert(NETWORK_IS_CLIENT(client));
	g_assert(PRIVATE(client)->handler == NULL);
	
	PRIVATE(client)->handler = handler;
	g_object_ref( G_OBJECT(handler));

        g_signal_connect_after( G_OBJECT(handler),
				 "recv-xml-message",
				 G_CALLBACK( recv_xml_message ),
				 client);

        g_signal_connect_after( G_OBJECT(handler),
				 "connection-closed",
				 G_CALLBACK( client_connection_closed ),
				 client);

}


NetworkMessageHandler * network_client_get_handler(NetworkClient * client) 
{
	g_assert(NETWORK_IS_CLIENT(client));

	return PRIVATE(client)->handler;
}

guint32 
network_client_get_id(NetworkClient * client) 
{
	return client->id;
}



static void 
recv_xml_message(NetworkMessageHandler * mmh,
		 guint32 client_id,
		 xmlDoc * doc,
		 NetworkClient * client) 
{
	
        xmlNode * root;
        guchar * message_name;

        root = doc->children;
        
        g_assert( g_str_equal(root->name,"message"));
        
        message_name = xmlGetProp(root,(guchar*)"name");
	g_print("NetworkClient : message name %s \n",message_name);
	if( g_str_equal( message_name,"ready_state") ) {
		g_print("new message state chaged \n");
		if( g_str_equal(doc->children->children->content,"true")) {
			PRIVATE(client)->state = NETWORK_CLIENT_READY;
		} else {
			PRIVATE(client)->state = NETWORK_CLIENT_NOT_READY;
		}        

		g_signal_emit( G_OBJECT(client),signals[STATE_CHANGED],0);

        } else if( g_str_equal( message_name,"disconnect") ) {

		g_signal_emit( G_OBJECT(client),signals[DISCONNECT_REQUEST],0);

        } else if( g_str_equal(message_name,"start_game")) {
		g_signal_emit( G_OBJECT(client),signals[START_REQUEST],0);
        } else if( g_str_equal(message_name,"game_created_ok")) {
		g_signal_emit( G_OBJECT(client),signals[GAME_CREATED_OK],0);
	} else if( g_str_equal(message_name,"number_of_players")) {
		int number;
            
		sscanf((gchar*)root->children->content,"%d",&number);
		g_signal_emit( G_OBJECT(client),signals[NUMBER_OF_PLAYERS],0,number);
	} else if( g_str_equal(message_name,"number_of_games")) {
		int number;
            
		sscanf((gchar*)root->children->content,"%d",&number);
		g_signal_emit( G_OBJECT(client),signals[NUMBER_OF_GAMES],0,number);
	}



	
}

void 
network_client_send_number_of_players(NetworkClient * client,
				     int n)
{
        xmlDoc * doc;
        xmlNode * root;
        xmlNode * text;
        
        doc = xmlNewDoc((guchar*)"1.0");
        root = xmlNewNode(NULL, (guchar*)"message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root, (guchar*)"name", (guchar*)"number_of_players");
        
        text = xmlNewText((guchar*)g_strdup_printf("%d",n));
        
        xmlAddChild(root,text);
	
        network_message_handler_send_xml_message(PRIVATE(client)->handler,
						 PRIVATE(client)->client_id,
						 doc);

        xmlFreeDoc(doc);

}


void 
network_client_send_number_of_games(NetworkClient * client,
				   int n) 
{
        xmlDoc * doc;
        xmlNode * root;
        xmlNode * text;
        
        doc = xmlNewDoc((guchar*)"1.0");
        root = xmlNewNode(NULL, (guchar*)"message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root, (guchar*)"name", (guchar*)"number_of_games");
        
        text = xmlNewText((guchar*)g_strdup_printf("%d",n));
        
        xmlAddChild(root,text);
	
        network_message_handler_send_xml_message(PRIVATE(client)->handler,
						 PRIVATE(client)->client_id,
						 doc);

        xmlFreeDoc(doc);
}

static void client_connection_closed(NetworkMessageHandler * mmh,
				     NetworkClient * client) 
{
	g_signal_emit( G_OBJECT(client),signals[DISCONNECTED],0);
}

static void 
network_client_instance_init(NetworkClient * self) 
{
	PRIVATE(self) = g_new0 (NetworkClientPrivate, 1);
	PRIVATE(self)->player = NULL;
	PRIVATE(self)->handler = NULL;
	PRIVATE(self)->state = NETWORK_CLIENT_NOT_READY;
}


static void
network_client_finalize (GObject * object) 
{
	NetworkClient * self;

	self = NETWORK_CLIENT(object);

	g_signal_handlers_disconnect_matched ( G_OBJECT(PRIVATE(self)->handler), 
					       G_SIGNAL_MATCH_DATA,
					       0, 0, NULL, NULL, self);


	if( PRIVATE(self)->handler != NULL) {
		g_object_unref( G_OBJECT(PRIVATE(self)->handler));		
	}

	if( PRIVATE(self)->player != NULL) {
		g_object_unref( G_OBJECT(PRIVATE(self)->player));		
	}

	g_free(PRIVATE(self));
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}

}


static void
network_client_class_init (NetworkClientClass	* network_client_class)
{
	GObjectClass	* g_object_class;

	parent_class = g_type_class_peek_parent(network_client_class);

	g_object_class = G_OBJECT_CLASS(network_client_class);
	g_object_class->finalize = network_client_finalize;

	// init signals
	signals[STATE_CHANGED]= 
		g_signal_new ("state-changed",
			      G_TYPE_FROM_CLASS (network_client_class),
			      G_SIGNAL_RUN_FIRST |
			      G_SIGNAL_NO_RECURSE,
			      G_STRUCT_OFFSET (NetworkClientClass, state_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0,NULL);
	

	signals[DISCONNECT_REQUEST]= 
		g_signal_new ("disconnect-request",
			      G_TYPE_FROM_CLASS (network_client_class),
			      G_SIGNAL_RUN_FIRST |
			      G_SIGNAL_NO_RECURSE,
			      G_STRUCT_OFFSET (NetworkClientClass, disconnect_request),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0,NULL);

	signals[START_REQUEST]= 
		g_signal_new ("start-request",
			      G_TYPE_FROM_CLASS (network_client_class),
			      G_SIGNAL_RUN_FIRST |
			      G_SIGNAL_NO_RECURSE,
			      G_STRUCT_OFFSET (NetworkClientClass, start_request),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0,NULL);


	signals[GAME_CREATED_OK]= 
		g_signal_new ("game-created-ok",
			      G_TYPE_FROM_CLASS (network_client_class),
			      G_SIGNAL_RUN_FIRST |
			      G_SIGNAL_NO_RECURSE,
			      G_STRUCT_OFFSET (NetworkClientClass, game_created_ok),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0,NULL);


	signals[NUMBER_OF_GAMES]= g_signal_new ("number-of-games",
						G_TYPE_FROM_CLASS (network_client_class),
						G_SIGNAL_RUN_FIRST |
						G_SIGNAL_NO_RECURSE,
						G_STRUCT_OFFSET (NetworkClientClass,change_number_of_games),
						NULL, NULL,
						g_cclosure_marshal_VOID__INT,
						G_TYPE_NONE,
						1, G_TYPE_INT);


	signals[NUMBER_OF_PLAYERS]= g_signal_new ("number-of-players",
						  G_TYPE_FROM_CLASS (network_client_class),
						  G_SIGNAL_RUN_FIRST |
						  G_SIGNAL_NO_RECURSE,
						  G_STRUCT_OFFSET (NetworkClientClass,change_number_of_players),
						  NULL, NULL,
						  g_cclosure_marshal_VOID__INT,
						  G_TYPE_NONE,
						  1, G_TYPE_INT);


	signals[DISCONNECTED]= 
		g_signal_new ("disconnected",
			      G_TYPE_FROM_CLASS (network_client_class),
			      G_SIGNAL_RUN_FIRST |
			      G_SIGNAL_NO_RECURSE,
			      G_STRUCT_OFFSET (NetworkClientClass, disconnected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0,NULL);
	
}


GType
network_client_get_type (void)
{
	static GType	type = 0;

	if (!type)
	{
		const GTypeInfo info = {
			sizeof (NetworkClientClass),
			NULL,	/* base initializer */
			NULL,	/* base finalizer */
			(GClassInitFunc)network_client_class_init,
			NULL,	/* class finalizer */
			NULL,	/* class data */
			sizeof (NetworkClient),
			1,
			(GInstanceInitFunc) network_client_instance_init,
			0
		};

		type = g_type_register_static (
				G_TYPE_OBJECT,
				"NetworkClient",
				&info,
				0);
	}

	return type;
}

