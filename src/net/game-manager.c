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

#include "game-manager.h"
#include "game.h"

#define PRIVATE(self) (self->private )
static GObjectClass* parent_class = NULL;

struct NetworkGameManagerPrivate 
{
	int number_of_games;
	int number_of_players;
	GList * clients;
	GList * waited_clients;
	GMutex * clients_lock;
	GHashTable * id_hash_table;
	gboolean started;
	GMutex * started_lock;

	gboolean can_join;
	NetworkGame * game;

	NetworkClient * owner;
};


static void client_disconnected(NetworkClient * client,
				NetworkGameManager * manager);


static void send_number_of_games(NetworkGameManager * manager);

static void send_number_of_players(NetworkGameManager * manager);

static void send_game_list(NetworkGameManager * manager);


static void send_game_joined(NetworkGameManager * manager,
			     NetworkClient * client);
      
static void client_state_changed(NetworkClient * client,
				 NetworkGameManager * manager);

static void client_request_start(NetworkClient * client,
				 NetworkGameManager * manager);

static void client_request_game_created_ok(NetworkClient * client,
					   NetworkGameManager * manager);

static void client_request_number_of_games(NetworkClient * client,
					   int n,
					   NetworkGameManager * manager);

static void client_request_number_of_players(NetworkClient * client,
					   int n,
					   NetworkGameManager * manager);

static void start_game(NetworkGameManager * manager);

NetworkGameManager *
network_game_manager_new  (void)
{
	NetworkGameManager * self;

	self = NETWORK_GAME_MANAGER( g_object_new(NETWORK_TYPE_GAME_MANAGER,NULL) );

	return self;
}

gboolean
network_game_manager_add_client(NetworkGameManager * manager,
				NetworkClient * client) 
{
	NetworkMessageHandler * handler;
	gboolean joined;
	g_mutex_lock( PRIVATE(manager)->started_lock);

	if( PRIVATE(manager)->can_join == TRUE) {
		handler = network_client_get_handler(client);

		g_mutex_lock(PRIVATE(manager)->clients_lock);


		g_signal_connect( G_OBJECT(client),
				  "disconnected",
				  G_CALLBACK(client_disconnected),
				  manager);

		if( PRIVATE(manager)->owner == NULL ) {
			PRIVATE(manager)->owner = client;
		}

		PRIVATE(manager)->clients = g_list_append(PRIVATE(manager)->clients,
							  client);

		g_hash_table_insert(PRIVATE(manager)->id_hash_table,
				    (gpointer)&client->id,
				    client);




		g_object_ref( G_OBJECT(client));
	
		g_signal_connect( G_OBJECT(client),
				  "state-changed",
				  G_CALLBACK(client_state_changed),
				  manager);

		g_signal_connect( G_OBJECT(client),
				  "start-request",
				  G_CALLBACK(client_request_start),
				  manager);

		g_signal_connect( G_OBJECT(client),
				  "game-created-ok",
				  G_CALLBACK(client_request_game_created_ok),
				  manager);

		g_signal_connect( G_OBJECT(client),
				  "number-of-games",
				  G_CALLBACK(client_request_number_of_games),
				  manager);


		g_signal_connect( G_OBJECT(client),
				  "number-of-players",
				  G_CALLBACK(client_request_number_of_players),
				  manager);
		g_mutex_unlock(PRIVATE(manager)->clients_lock);

		network_client_send_number_of_players(client,PRIVATE(manager)->number_of_players);
		network_client_send_number_of_games(client,PRIVATE(manager)->number_of_games);
		send_game_joined(manager,client);
		send_game_list(manager);
		joined = TRUE;
	} else {
		xmlDoc * doc;
		xmlNode * root;
		
		doc = xmlNewDoc((guchar*)"1.0");
		root = xmlNewNode(NULL, (guchar*)"message");
		xmlDocSetRootElement(doc, root);

		xmlNewProp(root,(guchar*)"name",(guchar*)"cant_join_game");
		
		
		network_message_handler_send_xml_message(network_client_get_handler(client),
							network_client_get_id(client),
							doc);
		
		xmlFreeDoc(doc);
		joined = FALSE;
		
	}
	
	
	g_mutex_unlock( PRIVATE(manager)->started_lock);
	return joined;
}


void 
network_game_manager_remove_client(NetworkGameManager * manager,
				   NetworkClient * client) 
{
	NetworkMessageHandler * handler;

	g_mutex_lock( PRIVATE(manager)->started_lock);

	handler = network_client_get_handler(client);

	g_mutex_lock(PRIVATE(manager)->clients_lock);


	if( PRIVATE(manager)->owner == client) {
		PRIVATE(manager)->owner = NULL;

		// maybe stop the game-manager??
	}

	PRIVATE(manager)->clients = g_list_remove(PRIVATE(manager)->clients,
						  client);

	g_hash_table_remove(PRIVATE(manager)->id_hash_table,
			    (gpointer)&client->id);

        g_signal_handlers_disconnect_matched(  G_OBJECT( client ),
                                               G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,manager);

	g_object_unref( G_OBJECT(client));
	
	g_mutex_unlock(PRIVATE(manager)->clients_lock);
		
	send_game_list(manager);
	g_mutex_unlock( PRIVATE(manager)->started_lock);
}

static void 
network_game_manager_instance_init(NetworkGameManager * self) 
{
	PRIVATE(self) = g_new0 (NetworkGameManagerPrivate, 1);			
	PRIVATE(self)->clients = NULL;
	PRIVATE(self)->id_hash_table = 
		g_hash_table_new(g_int_hash,g_int_equal);

	PRIVATE(self)->clients_lock = g_mutex_new();
	PRIVATE(self)->started = FALSE;
	PRIVATE(self)->can_join = TRUE;
	PRIVATE(self)->started_lock = g_mutex_new();
	PRIVATE(self)->number_of_games = 10;
	PRIVATE(self)->number_of_players = 2;
	PRIVATE(self)->owner = NULL;
}

static void
client_request_number_of_games(NetworkClient * client,
			       int n,
			       NetworkGameManager * manager)
{

	if( client == PRIVATE(manager)->owner && PRIVATE(manager)->started == FALSE) {

		PRIVATE(manager)->number_of_games = n;

		send_number_of_games(manager);
	}

}

static void 
client_request_number_of_players(NetworkClient * client,
				 int n,
				 NetworkGameManager * manager)
{
	if( client == PRIVATE(manager)->owner && PRIVATE(manager)->started == FALSE) {

		PRIVATE(manager)->number_of_players = n;

		send_number_of_players(manager);
	}

}


static void 
send_number_of_games_to_client(gpointer  data,gpointer user_data) {

	NetworkClient * client;
	NetworkGameManager * manager;

	client = NETWORK_CLIENT(data);
	manager = NETWORK_GAME_MANAGER(user_data);

	network_client_send_number_of_games(client, PRIVATE(manager)->number_of_games);
}
static void
send_number_of_games(NetworkGameManager * manager) 
{

        g_list_foreach(PRIVATE(manager)->clients,send_number_of_games_to_client,
                       manager);
	
}

static void 
send_number_of_players_to_client(gpointer data,gpointer user_data) {

	NetworkClient * client;
	NetworkGameManager * manager;

	client = NETWORK_CLIENT(data);
	manager = NETWORK_GAME_MANAGER(user_data);

	network_client_send_number_of_players(client, PRIVATE(manager)->number_of_players);
}

static void 
send_number_of_players(NetworkGameManager * manager) {
	g_list_foreach(PRIVATE(manager)->clients,send_number_of_players_to_client,
                       manager);
	
}




static void
send_game_joined(NetworkGameManager * manager,
		 NetworkClient * client) {
        
        
        xmlDoc * doc;
        xmlNode * text, * root;
        
        doc = xmlNewDoc((guchar*)"1.0");
        root = xmlNewNode(NULL, (guchar*)"message");
        xmlDocSetRootElement(doc, root);
        
        
	xmlNewProp(root,(guchar*)"name",(guchar*)"game_joined");
        
        // id of the game : one game, one id
        text = xmlNewText((guchar*)"1");
        
        xmlAddChild(root,text);
	
        network_message_handler_send_xml_message(network_client_get_handler(client),
                                                network_client_get_id(client),
                                                doc);
        
        xmlFreeDoc(doc);        
}


static void client_state_changed(NetworkClient * client,
				 NetworkGameManager * manager) {
	send_game_list(manager);
}

static void
network_game_manager_finalize (GObject * object) 
{
	NetworkGameManager * self;

	self = NETWORK_GAME_MANAGER(object);

	g_assert(g_hash_table_size(PRIVATE(self)->id_hash_table) == 0);

	g_assert(PRIVATE(self)->clients == NULL);

	g_mutex_free( PRIVATE(self)->clients_lock);
	g_mutex_free( PRIVATE(self)->started_lock);
	g_free(PRIVATE(self));

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}

}


void send_game_list_to_client(gpointer data,
                              gpointer user_data) {
        NetworkClient * client;
        xmlDoc * doc;

        client = NETWORK_CLIENT(data);
        doc =(xmlDoc *)user_data;

        network_message_handler_send_xml_message(network_client_get_handler(client),
                                                network_client_get_id(client),
                                                doc);

}


void send_game_list(NetworkGameManager * manager) {

        xmlDoc * doc;
        xmlNode * current, * root;
        GList * next;
        doc = xmlNewDoc((guchar*)"1.0");
        root = xmlNewNode(NULL,(guchar*)"message");

        xmlDocSetRootElement(doc, root);

        xmlNewProp(root,(guchar*)"name",(guchar*)"game_player_list");

	g_mutex_lock( PRIVATE(manager)->clients_lock);

        next = PRIVATE(manager)->clients;


        while( next != NULL) {
                xmlNode * text;
                NetworkClient * client;

                client = (NetworkClient *)next->data;
                
		current = xmlNewNode(NULL, (guchar*)"player");
                
		if( client == PRIVATE(manager)->owner) {
			xmlNewProp(current,(guchar*)"owner",(guchar*)"true");
		}

		xmlNewProp(current,(guchar*)"id", (guchar*)g_strdup_printf("%d",network_client_get_id(client)));
                xmlNewProp(current,(guchar*)"ready", ( network_client_get_state(client) ? (guchar*)"true" : (guchar*)"false"));
                text = xmlNewText((guchar*)network_player_get_name(network_client_get_player(client)));

                xmlAddChild(current,text);
                xmlAddChild(root,current);



                next = g_list_next(next);
        }

        g_list_foreach(PRIVATE(manager)->clients,send_game_list_to_client,
                       doc);
	

	g_mutex_unlock( PRIVATE(manager)->clients_lock);
        xmlFreeDoc(doc);
}

void send_start_game_to_client(gpointer data,
                              gpointer user_data)
{
        NetworkClient * nc;
        xmlDoc * doc;
        
        nc = (NetworkClient *)data;
        doc =(xmlDoc *)user_data;

	g_print("network-game-manager : send game_created message\n");
        network_message_handler_send_xml_message(network_client_get_handler(nc),
                                                network_client_get_id(nc),
                                                doc);

	
        
}

static void 
start_game(NetworkGameManager * manager) 
{


		xmlDoc * doc;
		xmlNode * text, * root;
		
		doc = xmlNewDoc((guchar*)"1.0");
		root = xmlNewNode(NULL, (guchar*)"message");
		
		xmlDocSetRootElement(doc, root);
		
		xmlNewProp(root,(guchar*)"name",(guchar*)"game_created");
		
		text = xmlNewText((guchar*)"1");
		xmlAddChild(root,text);
		
		
		PRIVATE(manager)->waited_clients = g_list_copy( PRIVATE(manager)->clients);
		
		g_list_foreach(PRIVATE(manager)->clients,
			       send_start_game_to_client,
			       doc);
	
		xmlFreeDoc(doc);
		PRIVATE(manager)->started = TRUE;
		PRIVATE(manager)->can_join = FALSE;
        

}

static void 
client_request_start(NetworkClient * client,
		     NetworkGameManager * manager) 
{


	g_mutex_lock( PRIVATE(manager)->started_lock);
	g_mutex_lock( PRIVATE(manager)->clients_lock);

	if( PRIVATE(manager)->started == FALSE && g_list_length( PRIVATE(manager)->clients) > 1
	    && PRIVATE(manager)->owner == client) {
		start_game(manager);
	}


	g_mutex_unlock( PRIVATE(manager)->clients_lock);

	g_mutex_unlock( PRIVATE(manager)->started_lock);
}


static gboolean
start_timeout(gpointer data) 
{

	NetworkGameManager * manager;

	manager = NETWORK_GAME_MANAGER(data);
	start_game(manager);

	return FALSE;
}

static void game_stopped(NetworkGame * game,
			 NetworkGameManager * manager) 
{

	g_print("NetworkGameManager : Game Stopped \n");
	
	g_signal_handlers_disconnect_matched(  game ,
					       G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,manager);
                      


	g_object_unref( G_OBJECT(PRIVATE(manager)->game));
	PRIVATE(manager)->game = NULL;

	PRIVATE(manager)->started = FALSE;

	if( PRIVATE(manager)->started == FALSE && g_list_length( PRIVATE(manager)->clients) > 1 ) {
		g_print("length %d\n", g_list_length(PRIVATE(manager)->clients));
		// restart after 3 seconde !
		// if the number of winned game < number_of_games
		g_timeout_add(3000,start_timeout,manager);
	}
}

static void 
client_request_game_created_ok(NetworkClient * client,
			       NetworkGameManager * manager) 
{

	// TODO test owner ?

	// test game state

	g_mutex_lock( PRIVATE(manager)->started_lock);
	g_mutex_lock( PRIVATE(manager)->clients_lock);

	PRIVATE(manager)->waited_clients =
		g_list_remove(PRIVATE(manager)->waited_clients,client);

	g_mutex_unlock( PRIVATE(manager)->clients_lock);

	g_mutex_unlock( PRIVATE(manager)->started_lock);

	if( g_list_length(PRIVATE(manager)->waited_clients) == 0) {
		PRIVATE(manager)->game = network_game_new(PRIVATE(manager)->clients);
		
		g_signal_connect( G_OBJECT( PRIVATE(manager)->game),
				  "game-stopped",
				  G_CALLBACK(game_stopped),
				  manager);
	
		network_game_start(PRIVATE(manager)->game);
	

	}

}

static void 
client_disconnected(NetworkClient * client,
		    NetworkGameManager * manager) {

	network_game_manager_remove_client(manager,
					   client);

}

static void
network_game_manager_class_init (NetworkGameManagerClass	* network_game_manager_class)
{
	GObjectClass	* g_object_class;

	parent_class = g_type_class_peek_parent(network_game_manager_class);

	g_object_class = G_OBJECT_CLASS(network_game_manager_class);
	g_object_class->finalize = network_game_manager_finalize;

}


GType
network_game_manager_get_type (void)
{
	static GType	type = 0;

	if (!type)
	{
		const GTypeInfo info = {
			sizeof (NetworkGameManagerClass),
			NULL,	/* base initializer */
			NULL,	/* base finalizer */
			(GClassInitFunc)network_game_manager_class_init,
			NULL,	/* class finalizer */
			NULL,	/* class data */
			sizeof (NetworkGameManager),
			1,
			(GInstanceInitFunc) network_game_manager_instance_init,
			0
		};

		type = g_type_register_static (
				G_TYPE_OBJECT,
				"NetworkGameManager",
				&info,
				0);
	}

	return type;
}

