/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* mn-server.c - 
 * Copyright (C) 2004 Laurent Belmonte <lolo3d@tuxfamily.org>
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

#include <libxml/tree.h>
#include <gtk/gtk.h>

#include "mn-game-manager.h"
#include "monkey-network-game.h"
#include "monkey-message-handler.h"

#define SERVER_PORT 6666

struct MnGameManagerPrivate {
        GHashTable *    pending_clients;
        GHashTable *    clients_by_name;
        GHashTable *    clients_by_id;
        GHashTable *    clients_by_handler;

        GList *         games;
        GMutex *        global_lock;
        gint            socket;
        gint            port;
        gboolean        is_running;
        GThread *       main_thread;
        guint32         current_client_id;
        gboolean        init_ok;
        NetworkGame *   ng;
};

#define PRIVATE( MnGameManager ) (MnGameManager->private)

static GObjectClass* parent_class = NULL;

void mn_game_manager_finalize(GObject *);

gboolean connect_client(MnGameManager * manager,
                    NetworkClient * client);

void disconnect_client(MnGameManager * manager,
                       NetworkClient * client);

void send_game_list(MnGameManager * manager);

void start_game(MnGameManager * manager,NetworkGame * game);

gboolean init_server_socket(MnGameManager * manager);



static NetworkClient * get_network_client_by_handler(MnGameManager * manager,MonkeyMessageHandler * handler);
static NetworkClient * get_network_client_by_client_id(MnGameManager * manager,int client_id);

static gboolean is_unique_client_name(MnGameManager * manager,
                                      const gchar * client_name);

static void add_client_to_game(MnGameManager * manager,
                               NetworkGame * ng,
                               NetworkClient * client);


static void remove_client_to_game(MnGameManager * manager,
                                  NetworkGame * ng,
                                  NetworkClient * client);

gboolean connect_client(MnGameManager * manager,
                    NetworkClient * client) {

        if( is_unique_client_name(manager,client->client_name) ) {
                g_hash_table_insert( PRIVATE(manager)->clients_by_handler,
                                     (gpointer)client->handler,
                                     client);
                
                g_hash_table_insert( PRIVATE(manager)->clients_by_name,
                                     (gpointer)client->client_name,
                                     client);
                
                g_hash_table_insert( PRIVATE(manager)->clients_by_id,
                                     (gpointer)&(client->client_id),
                                     client);
                
                g_hash_table_remove( PRIVATE(manager)->pending_clients,
                                     (gpointer)&(client->client_id));
                return TRUE;
        } else {
                return FALSE;
        }
}

void disconnect_client(MnGameManager * manager,
                       NetworkClient * client) {
        
        if( g_hash_table_lookup(PRIVATE(manager)->pending_clients,
                                &(client->client_id)) != NULL) {

                // just a pending client

                g_hash_table_remove( PRIVATE(manager)->pending_clients,
                                     &(client->client_id));
                
        } else {

                if( client->game != NULL) {
                        remove_client_to_game(manager,client->game,client);
                }

                
                
                g_hash_table_remove( PRIVATE(manager)->clients_by_name,
                                     (gpointer)client->client_name);
                
                g_hash_table_remove( PRIVATE(manager)->clients_by_id,
                                     (gpointer)&(client->client_id));
                
                 

        }




        monkey_message_handler_disconnect(client->handler);
                
}



void client_connection_closed(MonkeyMessageHandler * mmh,
                              MnGameManager * manager) {


        NetworkClient * client;

        client = get_network_client_by_handler(manager,mmh);
        g_print("client connection closed \n");
        if( client != NULL ) {
                g_print("client connection closed %s\n",client->client_name);

                disconnect_client(manager,client);
                g_hash_table_remove( PRIVATE(manager)->clients_by_handler,
                                     (gpointer)client->handler);

                g_object_unref( G_OBJECT( client->handler) );
                client->handler = NULL;
                
                g_free(client->client_name);
                
                g_free(client);

          
        }
        
}

static gboolean is_unique_client_name(MnGameManager * manager,
                                      const gchar * client_name) {

        return TRUE;
}

void send_client_join_game(MnGameManager * manager,
                           NetworkGame * ng,
                           NetworkClient * nc) {
        
        
        xmlDoc * doc;
        xmlNode * text, * root;
        
        doc = xmlNewDoc("1.0");
        root = xmlNewNode(NULL,
                          "message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,"name","game_joined");
        
        // id of the game : one game, one id
        text = xmlNewText("1");
        
        xmlAddChild(root,text);

        monkey_message_handler_send_xml_message(nc->handler,
                                                nc->client_id,
                                                doc);
        
        xmlFreeDoc(doc);        
}

static void add_client_to_game(MnGameManager * manager,
                               NetworkGame * ng,
                               NetworkClient * client) {

        ng->clients = g_list_append(ng->clients,
                                    client);


        client->game = ng;
        if( ng->game_owner == NULL ) {
                ng->game_owner = client;
        }

        send_client_join_game(manager,ng,client);
        send_game_list(manager);
}

static void remove_client_to_game(MnGameManager * manager,
                                  NetworkGame * ng,
                                  NetworkClient * client) {

        ng->clients = g_list_remove(ng->clients,
                                    client);

        if( ng->game_owner == client) {
                if( ng->clients != NULL ) {
                        ng->game_owner = (NetworkClient *)ng->clients->data;
                } else {
                        ng->game_owner = NULL;
                }       
        }
        
        send_game_list(manager);
}

static NetworkClient * get_network_client_by_handler(MnGameManager * manager,MonkeyMessageHandler * handler) {
        return (NetworkClient *) g_hash_table_lookup(PRIVATE(manager)->clients_by_handler,handler);
}


static NetworkClient * get_network_client_by_client_id(MnGameManager * manager,int client_id) {
        return (NetworkClient *) g_hash_table_lookup(PRIVATE(manager)->clients_by_id,&client_id);
}


void send_init_reply(MnGameManager * manager,
                     NetworkClient * nc,
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

        monkey_message_handler_send_xml_message(nc->handler,
                                                nc->client_id,
                                                doc);
        
        xmlFreeDoc(doc);        
}

void recv_init_message(MnGameManager * manager,
                       NetworkClient * nc,
                       xmlDoc * doc) {


        
        xmlNode * root;

        NetworkGame * ng;
                        
        char * client_name;

        root = doc->children;
        
        client_name = root->children->children->content;
        
        
        nc->client_name = client_name;
        
        ng = PRIVATE(manager)->ng;
                                
        if( connect_client(manager,nc) ) {
                
                send_init_reply(manager,nc,"ok");
                add_client_to_game(manager,ng,nc);                                        
        } else {
                send_init_reply(manager,nc,"not ok");
                disconnect_client(manager,nc);
        }
                        

}

void recv_xml_message(MonkeyMessageHandler * mmh,
                      guint32 client_id,
                      xmlDoc * doc,
                      MnGameManager * manager) {

        
        xmlNode * root;
        char * message_name;
        NetworkClient * nc;
        

        root = doc->children;
        
        g_assert( g_str_equal(root->name,"message"));
        
        
        message_name = xmlGetProp(root,"name");
        g_print("message name %s",message_name);
        if( g_str_equal( message_name,"init") ){
                nc = (NetworkClient *) g_hash_table_lookup(PRIVATE(manager)->pending_clients,
                                                           &client_id);
        
                if( nc == NULL) {
                        g_print("Bad client %d\n",client_id);
                        return;
                }
                
                recv_init_message(manager,nc,doc);
        } else if( g_str_equal( message_name,"ready_state") ) {
                nc = get_network_client_by_client_id(manager,client_id);
                if( nc != NULL ) {
                        if( g_str_equal(doc->children->children->content,"true")) {
                                nc->ready = TRUE;
                                send_game_list(manager);
                        } else {
                                nc->ready = FALSE;
                                send_game_list(manager);
                        }
                } else {
                        g_print("nc is NULL !! ? \n");
                }
                
        } else if( g_str_equal( message_name,"disconnect") ) {
                nc = get_network_client_by_client_id(manager,client_id);
                if( nc != NULL ) {
                        disconnect_client(manager,nc);                        
                } else {
                        g_print("nc is NULL !! ? \n");
                }

        } else if( g_str_equal(message_name,"start_game")) {
                int game_id;
                nc = get_network_client_by_client_id(manager,client_id);
                sscanf(root->children->content,"%d",&game_id);
                g_print("start game id %d\n",game_id);
                if( nc != NULL ) {
                        start_game(manager,PRIVATE(manager)->ng);
                } else {
                        g_print("nc is NULL !! ? \n");
                }

        }


        
}


void send_game_list_to_client(gpointer data,
                              gpointer user_data) {
        NetworkClient * nc;
        xmlDoc * doc;

        nc = (NetworkClient *)data;
        doc =(xmlDoc *)user_data;
        monkey_message_handler_send_xml_message(nc->handler,
                                                nc->client_id,
                                                doc);

}

void send_game_list(MnGameManager * manager) {

        xmlDoc * doc;
        xmlNode * current, * root;
        NetworkGame * ng;
        GList * next;
        doc = xmlNewDoc("1.0");
        root = xmlNewNode(NULL,
                          "message");

        xmlDocSetRootElement(doc, root);
         

        xmlNewProp(root,"name","game_player_list");

        ng = PRIVATE(manager)->ng;

        next = ng->clients;

        while( next != NULL) {
                xmlNode * text;
                NetworkClient * client;

                client = (NetworkClient *)next->data;
                current = xmlNewNode(NULL,
                                     "player");
                
                if( client == ng->game_owner) {
                        xmlNewProp(current,"owner","true");
                }

                xmlNewProp(current,"ready", ( client->ready ? "true" : "false"));
                text = xmlNewText(client->client_name);

                xmlAddChild(current,text);
                xmlAddChild(root,current);
                next = g_list_next(next);
        }

        g_list_foreach(ng->clients,send_game_list_to_client,
                       doc);


        xmlFreeDoc(doc);
}


void send_start_game_to_client(gpointer data,
                              gpointer user_data) {
        NetworkClient * nc;
        xmlDoc * doc;
        
        nc = (NetworkClient *)data;
        doc =(xmlDoc *)user_data;
        monkey_message_handler_send_xml_message(nc->handler,
                                                nc->client_id,
                                                doc);
        
}

void send_start_game(MnGameManager * manager,
                NetworkGame * game) {
        xmlDoc * doc;
        xmlNode * text, * root;

        doc = xmlNewDoc("1.0");
        root = xmlNewNode(NULL,
                          "message");

        xmlDocSetRootElement(doc, root);
         

        xmlNewProp(root,"name","game_started");

        text = xmlNewText("1");
        xmlAddChild(root,text);



        g_list_foreach(game->clients,send_start_game_to_client,
                       doc);

        xmlFreeDoc(doc);
        
}

void start_game(MnGameManager * manager,
                NetworkGame * game) {

        send_start_game(manager,game);
        game->game = monkey_network_game_new(game);
}

guint32 get_next_client_id(MnGameManager * manager) {
        PRIVATE(manager)->current_client_id++;
        return PRIVATE(manager)->current_client_id;
}

MnGameManager *mn_game_manager_new() {
        MnGameManager * manager;
        
        manager = 
                MN_GAME_MANAGER(g_object_new(TYPE_MN_GAME_MANAGER
                                                    , NULL));

        PRIVATE(manager)->games = NULL;
        PRIVATE(manager)->global_lock = g_mutex_new();



        PRIVATE(manager)->ng =(NetworkGame *) g_malloc( sizeof(NetworkGame));
        PRIVATE(manager)->ng->game_id = 1;
        PRIVATE(manager)->ng->game = NULL;
        PRIVATE(manager)->ng->clients = NULL;
        PRIVATE(manager)->ng->game_owner = NULL;

        PRIVATE(manager)->pending_clients = g_hash_table_new(g_int_hash,g_int_equal);

        PRIVATE(manager)->clients_by_handler =
                g_hash_table_new(g_direct_hash,g_direct_equal);

        PRIVATE(manager)->clients_by_name = g_hash_table_new(g_str_hash,g_str_equal);

        PRIVATE(manager)->clients_by_id = g_hash_table_new(g_int_hash,g_int_equal);
                
        PRIVATE(manager)->init_ok = init_server_socket(manager);
        return manager;        
}

gboolean init_server_socket(MnGameManager * manager) {
        
	struct sockaddr_in sock_client;
  	socklen_t length;

	if ((PRIVATE(manager)->socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                perror("server socket create error ");
                return FALSE;
        }
        
	bzero((char *) &sock_client, sizeof(sock_client));
	sock_client.sin_family = AF_INET;
	sock_client.sin_addr.s_addr = INADDR_ANY;
	
        sock_client.sin_port = htons(SERVER_PORT);
	
        if (bind(PRIVATE(manager)->socket, 
                 (struct sockaddr *)&sock_client, 
                 sizeof(sock_client)) == -1) {
                
		perror("bind server socket error ");
		return FALSE;
	}

	
        if (listen(PRIVATE(manager)->socket, 4) == -1) {
                perror("server socket listen() error ");
                return FALSE;
        }
        
        length = sizeof(sock_client);
        
        if (getsockname(PRIVATE(manager)->socket,(struct sockaddr *) &sock_client, &length) == -1) {
                perror("getsockname() ");
                return FALSE;
        }
        
	PRIVATE(manager)->port = ntohs(sock_client.sin_port);
        return TRUE;			
}

void create_client(MnGameManager * manager,int sock) {
        MonkeyMessageHandler * handler;
        xmlDoc * doc;
        xmlNode * root;

        NetworkClient * nc = g_malloc( sizeof( NetworkClient) );
        
        
        handler = monkey_message_handler_new( sock);
        
        g_signal_connect( G_OBJECT(handler),
                          "connection-closed",
                          G_CALLBACK(client_connection_closed),
                          manager);

        nc->handler = handler;
        nc->game = NULL;
        g_signal_connect( G_OBJECT(handler),
                          "recv-xml-message",
                          G_CALLBACK( recv_xml_message ),
                          manager);

        nc->client_id = get_next_client_id(manager);
        nc->ready = FALSE;

        
        doc = xmlNewDoc("1.0");
        root = xmlNewNode(NULL,
                          "message");
        xmlDocSetRootElement(doc, root);

        xmlNewProp(root,"name","init_request");
        
        monkey_message_handler_send_xml_message( handler, 
                                                 nc->client_id,
                                                 doc);


        xmlFreeDoc(doc);

        g_hash_table_insert( PRIVATE(manager)->pending_clients,
                             (gpointer )&(nc->client_id),
                             nc);


        monkey_message_handler_start_listening( handler);

        // the client have 5second to reply to the init message

        
}

void * server_main_loop(MnGameManager * manager) {
        
        while( PRIVATE(manager)->is_running ) {
                struct sockaddr_in sock_client;
                int sock;
                int lg_info = sizeof(sock_client);
				
                g_log(G_LOG_DOMAIN,G_LOG_LEVEL_DEBUG,"Waiting for new connection\n");
                if ((sock = accept(PRIVATE(manager)->socket, (struct sockaddr *) &sock_client, &lg_info)) == -1) {	
                        perror("accept() ");
                        close(sock);
                        exit(-1);
                }

                create_client( manager,sock);
                
                g_log(G_LOG_DOMAIN,G_LOG_LEVEL_DEBUG,"New client connected, connection accepted from %s\n",inet_ntoa(sock_client.sin_addr));
        }
        
        return 0;
}

void mn_game_manager_join(MnGameManager * manager) {
        g_thread_join( PRIVATE(manager)->main_thread);
}

gboolean mn_game_manager_start_server(MnGameManager * manager) {
        GError ** error;

        g_assert(IS_MN_GAME_MANAGER(manager));

        if( !PRIVATE(manager)->init_ok ) {
                g_error("server init not ok");
                return FALSE;
        }

        PRIVATE(manager)->is_running = TRUE;
        error = NULL;
        PRIVATE(manager)->main_thread =
                g_thread_create((GThreadFunc) server_main_loop, manager, TRUE, error);
        
        if( error != NULL) {
                g_error("error on creating the main loop");
                return FALSE;
        }

        return TRUE;
}


void mn_game_manager_finalize(GObject *object) {
        MnGameManager * mng = (MnGameManager *) object;
	
        g_free(PRIVATE(mng));

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }		
}

static void mn_game_manager_instance_init(MnGameManager * mng) {
        mng->private =g_new0 (MnGameManagerPrivate, 1);
}

static void mn_game_manager_class_init (MnGameManagerClass *klass) {
        GObjectClass* object_class;
        
        parent_class = g_type_class_peek_parent(klass);
        object_class = G_OBJECT_CLASS(klass);
        object_class->finalize = mn_game_manager_finalize;
}

GType mn_game_manager_get_type(void) {
        static GType mn_game_manager_type = 0;
        
        if (!mn_game_manager_type) {
                static const GTypeInfo mn_game_manager_info = {
                        sizeof(MnGameManagerClass),
                        NULL,           /* base_init */
                        NULL,           /* base_finalize */
                        (GClassInitFunc) mn_game_manager_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof(MnGameManager),
                        1,              /* n_preallocs */
                        (GInstanceInitFunc) mn_game_manager_instance_init,
                };
                
                mn_game_manager_type = g_type_register_static(G_TYPE_OBJECT,
                                                                     "MnGameManager",
                                                                     &mn_game_manager_info, 0
                                                                     );

        }
        
        return mn_game_manager_type;
}
