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

#include <gtk/gtk.h>

#include "mn-game-manager.h"
#include "monkey-network-game.h"
#include "monkey-message-handler.h"

#define SERVER_PORT 6666
typedef struct NetworkClient {
        guint          client_id;
        gchar *        client_name;
        MonkeyMessageHandler * handler;
} NetworkClient;

typedef struct NetworkGame {
        guint                 game_id;
        MonkeyNetworkGame *   game;
        GList *               clients;
        NetworkClient *       game_owner;
} NetworkGame;

struct MnGameManagerPrivate {
        GHashTable *    pending_clients;
        GHashTable *    clients;
        GList *         games;
        GMutex *        global_lock;
        gint            socket;
        gint            port;
        gboolean        is_running;
        GThread *       main_thread;
        guint32         current_client_id;
        gboolean        init_ok;
};

#define PRIVATE( MnGameManager ) (MnGameManager->private)

static GObjectClass* parent_class = NULL;

void mn_game_manager_finalize(GObject *);
void init_client(MonkeyMessageHandler * mmh,
                 guint32 client_id,
                 gchar * message);

gboolean init_server_socket(MnGameManager * manager);



void init_client(MonkeyMessageHandler * mmh,
                 guint32 client_id,
                 gchar * message) {

        g_print("Message client %d, message :\n%s\n",client_id,message);
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

        PRIVATE(manager)->clients = g_hash_table_new(g_int_hash,g_int_equal);

        PRIVATE(manager)->pending_clients = g_hash_table_new(g_int_hash,g_int_equal);
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

        NetworkClient * nc = g_malloc( sizeof( NetworkClient) );
        
        
        handler = monkey_message_handler_new( sock);
        
        nc->handler = handler;

        g_signal_connect( G_OBJECT(handler),
                          "recv-message",
                          G_CALLBACK( init_client ),
                          manager);

        nc->client_id = get_next_client_id(manager);

        monkey_message_handler_send_message( handler, 
                                             nc->client_id,
                                             "INIT",5);


        g_hash_table_insert( PRIVATE(manager)->pending_clients,
                             &(nc->client_id),
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