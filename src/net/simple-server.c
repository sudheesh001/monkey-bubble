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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <glib/gthread.h>
#include <sys/time.h>
#include <time.h>


#include <glib-object.h>
#include <glib.h>

#include "simple-server.h"
#include "monkey-message-handler.h"
#include "game-manager.h"
#include "client.h"
#include "player.h"

#define MAX_WAITING_CONN 1
#define SERVER_PORT 6666

#define PRIVATE(self) (self->private )
static GObjectClass* parent_class = NULL;

static gboolean bind_socket(NetworkSimpleServer * self);
static void close_socket(NetworkSimpleServer * self);
static void * accept_loop(NetworkSimpleServer * self);

static void remove_handler(NetworkSimpleServer * self,
			   MonkeyMessageHandler * hander);
static void add_handler(NetworkSimpleServer * self,
			MonkeyMessageHandler * hander);

static void create_client(NetworkSimpleServer * self,
			  MonkeyMessageHandler * handler,
			  guint32 client_id,
			  const char * name);

static guint32 get_next_id(NetworkSimpleServer * self);

struct _NetworkSimpleServerPrivate 
{
	gint main_socket;
	gboolean is_running;
	GThread * main_thread;
	GList * handlers;
	GMutex * handlers_lock;

	NetworkGameManager * game_manager;

	guint32 current_id;

};

NetworkSimpleServer *
network_simple_server_new  (void)
{
	NetworkSimpleServer * self;

	self = NETWORK_SIMPLE_SERVER( g_object_new(NETWORK_TYPE_SIMPLE_SERVER,NULL) );


	return self;
}


/**
 * bind the socket and start the accept thread
 **/
gboolean
network_simple_server_start(NetworkSimpleServer * self)
{
	if( bind_socket(self) ) {
		// main_socket is ready to accept connection
		GError ** error = NULL;


		PRIVATE(self)->is_running = TRUE;		
		PRIVATE(self)->main_thread =
			g_thread_create((GThreadFunc) accept_loop, self, TRUE, error);
        
		if( error != NULL) {
			g_error("error on creating the main loop");
			close_socket( self);

			PRIVATE(self)->is_running = FALSE;
			return FALSE;
		}
		
		return TRUE;
	} else {	
		return FALSE;
	}
}

static gboolean
bind_socket(NetworkSimpleServer * self) 
{
	
	struct sockaddr_in sock_client;
	gint sock;

	g_assert( PRIVATE(self)->main_socket == -1);
	
	g_assert(NETWORK_IS_SIMPLE_SERVER(self));

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("server socket create error ");
                return FALSE;
        }
        
	bzero((char *) &sock_client, sizeof(sock_client));
	sock_client.sin_family = AF_INET;
	sock_client.sin_addr.s_addr = INADDR_ANY;
	
        sock_client.sin_port = htons(SERVER_PORT);
	
        if (bind(sock, 
                 (struct sockaddr *)&sock_client, 
                 sizeof(sock_client)) == -1) {
                
		close(sock);
		perror("bind server socket error ");
		return FALSE;
	}

	
        if (listen( sock, MAX_WAITING_CONN) == -1) {
		close(sock);
                perror("server socket listen() error ");
                return FALSE;
        }
        
        PRIVATE(self)->main_socket = sock;
	return TRUE;	
}

static void
close_socket(NetworkSimpleServer * self) 
{
	g_assert( PRIVATE(self)->main_socket != -1);
	close(PRIVATE(self)->main_socket);
	PRIVATE(self)->main_socket = -1;
}

static void
client_connection_closed(MonkeyMessageHandler * handler,
			 NetworkSimpleServer * self) {


        
	g_print("client connection closed \n");
	remove_handler( self, handler);
        
}

static void add_handler(NetworkSimpleServer * self,
			MonkeyMessageHandler * handler) 
{
	g_mutex_lock( PRIVATE( self)->handlers_lock);
	PRIVATE(self)->handlers = g_list_append( PRIVATE(self)->handlers,
						 handler);

	g_object_ref( G_OBJECT(handler));
	g_mutex_unlock( PRIVATE( self)->handlers_lock);
}

static void remove_handler(NetworkSimpleServer * self,
			MonkeyMessageHandler * handler) 
{
	g_mutex_lock( PRIVATE( self)->handlers_lock);
	PRIVATE(self)->handlers = g_list_remove( PRIVATE(self)->handlers,
						 handler);


	g_object_unref( G_OBJECT(handler));
	g_mutex_unlock( PRIVATE( self)->handlers_lock);
}

static void
recv_xml_message(MonkeyMessageHandler * handler,
		 guint32 client_id,
		 xmlDoc * doc,
		 NetworkSimpleServer * self) {

        xmlNode * root;
        char * message_name;
        

        root = doc->children;
        
        g_assert( g_str_equal(root->name,"message"));
        
        
        message_name = xmlGetProp(root,"name");
        g_print("NetworkSimpleServer: message name %s\n",message_name);

	if( g_str_equal( message_name,"init") ){		
		g_print("connected %d\n",client_id);
		create_client(self,handler,client_id,
			      root->children->children->content);
	}
	
}

static void
send_init_reply(NetworkSimpleServer * server,
		NetworkClient * client,
		const char * status) {

	
        xmlDoc * doc;
        xmlNode * text, * root;
        
        doc = xmlNewDoc("1.0");
        root = xmlNewNode(NULL,
                          "message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,"name","init_reply");
        
        
        text = xmlNewText(status);
        
        xmlAddChild(root,text);

        monkey_message_handler_send_xml_message(network_client_get_handler(client),
						network_client_get_id(client),
                                                doc);
        
        xmlFreeDoc(doc);
}

static void
create_client(NetworkSimpleServer * self,
	      MonkeyMessageHandler * handler,
	      guint32 client_id,
	      const char * name)
{

	NetworkClient * client;
	NetworkPlayer * player;

	client = network_client_new(client_id);

	network_client_set_handler(client,handler);

	player = network_player_new(name);

	network_client_set_player(client,player);

	send_init_reply(self,
			client,
			"ok");

	network_game_manager_add_client(PRIVATE(self)->game_manager,
					client);

	g_object_unref( player);
	g_object_unref( client);

}

static void
connect_client(NetworkSimpleServer * self,gint sock) 
{
	MonkeyMessageHandler * handler;
	guint32 client_id;
        xmlDoc * doc;
        xmlNode * root;

	handler = monkey_message_handler_new( sock);
        
        g_signal_connect( G_OBJECT(handler),
                          "connection-closed",
                          G_CALLBACK(client_connection_closed),
                          self);

        g_signal_connect( G_OBJECT(handler),
                          "recv-xml-message",
                          G_CALLBACK( recv_xml_message ),
                          self);

	add_handler(self,handler);


	g_object_unref( G_OBJECT(handler));

	
        client_id = get_next_id(self);
        
        
        doc = xmlNewDoc("1.0");
        root = xmlNewNode(NULL,
                          "message");
        xmlDocSetRootElement(doc, root);

        xmlNewProp(root,"name","init_request");

        monkey_message_handler_send_xml_message( handler, 
                                                 client_id,
                                                 doc);


        xmlFreeDoc(doc);

        monkey_message_handler_start_listening( handler);

}

void *
accept_loop(NetworkSimpleServer * self) 
{
	
	struct sockaddr_in sock_client;
	int sock;
	int lg_info;
	
	lg_info = sizeof(sock_client);
	while( (sock = accept(PRIVATE(self)->main_socket, (struct sockaddr *) &sock_client, &lg_info)) != -1) {
		connect_client( self,sock);
	}

	return 0;
                
}

static guint32
get_next_id(NetworkSimpleServer * self) 
{
        PRIVATE(self)->current_id++;
        return PRIVATE(self)->current_id;

}

static void 
network_simple_server_instance_init(NetworkSimpleServer * self) 
{
	PRIVATE(self) = g_new0 (NetworkSimpleServerPrivate, 1);			
	PRIVATE(self)->main_socket = -1;
	PRIVATE(self)->is_running = FALSE;
	PRIVATE(self)->main_thread = NULL;
	PRIVATE(self)->handlers = NULL;
	PRIVATE(self)->handlers_lock = g_mutex_new();

	PRIVATE(self)->current_id = 0;

	PRIVATE(self)->game_manager = network_game_manager_new();

}


static void
network_simple_server_finalize (GObject * object) 
{
	NetworkSimpleServer * self;

	self = NETWORK_SIMPLE_SERVER(object);
	
	g_mutex_free( PRIVATE(self)->handlers_lock);

	close_socket(self);

	g_free(PRIVATE(self));
	
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}

}


static void
network_simple_server_class_init (NetworkSimpleServerClass	* network_simple_server_class)
{
	GObjectClass	* g_object_class;

	parent_class = g_type_class_peek_parent(network_simple_server_class);

	g_object_class = G_OBJECT_CLASS(network_simple_server_class);
	g_object_class->finalize = network_simple_server_finalize;

}


GType
network_simple_server_get_type (void)
{
	static GType	type = 0;

	if (!type)
	{
		const GTypeInfo info = {
			sizeof (NetworkSimpleServerClass),
			NULL,	/* base initializer */
			NULL,	/* base finalizer */
			(GClassInitFunc)network_simple_server_class_init,
			NULL,	/* class finalizer */
			NULL,	/* class data */
			sizeof (NetworkSimpleServer),
			1,
			(GInstanceInitFunc) network_simple_server_instance_init,
			0
		};

		type = g_type_register_static (
				G_TYPE_OBJECT,
				"NetworkSimpleServer",
				&info,
				0);
	}

	return type;
}

