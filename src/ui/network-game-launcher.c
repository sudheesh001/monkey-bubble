/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* monkey-server.c - 
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
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

#include <gtk/gtk.h>

#include "monkey-message-handler.h"

#include <glade/glade.h>
#include "network-game-launcher.h"

typedef struct NetworkClient {
        gchar *        client_name;
        gboolean       game_owner;
} NetworkClient;


typedef struct NetworkGame {
        GList *               clients;
} NetworkGame;

struct NetworkGameLauncherPrivate {
        GladeXML * glade_xml;
        GtkWidget * window;
        gchar * server_name;
        MonkeyMessageHandler * handler;
        int client_id;

        GtkLabel * connection_label;
        GtkListStore * players_list;
        NetworkGame * game;
};



#define PRIVATE( NetworkGameLauncher ) (NetworkGameLauncher->private)

static GObjectClass* parent_class = NULL;

void network_game_launcher_finalize(GObject *);

static void connect_server_signal(gpointer    callback_data,
                           guint       callback_action,
                           GtkWidget  *widget);


static void quit_server_signal(gpointer    callback_data,
                           guint       callback_action,
                           GtkWidget  *widget);

static gboolean connect_server(NetworkGameLauncher * launcher);

static void set_status_message(NetworkGameLauncher * launcher,
                               const gchar * message);

static void set_sensitive(GtkWidget * w,
                          gboolean s);

static void update_players_list(NetworkGameLauncher * launcher);

void recv_network_xml_message(MonkeyMessageHandler * mmh,
                              guint32 client_id,
                              xmlDoc * message,
                              gpointer * p);

NetworkGameLauncher *network_game_launcher_new() {
        NetworkGameLauncher * ngl;
        GtkWidget * item;

        GtkTreeViewColumn * column;
        GtkListStore * list;

        ngl = 
                NETWORK_GAME_LAUNCHER(g_object_new(TYPE_NETWORK_GAME_LAUNCHER
                                                    , NULL));

        PRIVATE(ngl)->server_name = NULL;

        PRIVATE(ngl)->glade_xml = glade_xml_new("/home/laurentb/cvs/monkey-bubble/data/netgame.glade","network_window",NULL);
        
        PRIVATE(ngl)->window = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "network_window");

       
        item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "go_button");
        g_signal_connect_swapped( item,"clicked",GTK_SIGNAL_FUNC(connect_server_signal),ngl);


        item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "quit_button");
        g_signal_connect_swapped( item,"clicked",GTK_SIGNAL_FUNC(quit_server_signal),ngl);

        
        PRIVATE(ngl)->connection_label = GTK_LABEL(glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "connection_state_label"));
        
       gtk_widget_set_sensitive( glade_xml_get_widget( PRIVATE(ngl)->glade_xml
                                                       , "connected_game_hbox"),
                                 FALSE);


       item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml,"players_treeview");
       list = gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_BOOLEAN);

        column = gtk_tree_view_column_new_with_attributes("Player name",gtk_cell_renderer_text_new(),
                                                          "text",0);

        gtk_tree_view_append_column (GTK_TREE_VIEW (item), column);

        column = gtk_tree_view_column_new();

        column = gtk_tree_view_column_new_with_attributes("Owner",gtk_cell_renderer_toggle_new(),
                                                          "active",1);

        gtk_tree_view_append_column (GTK_TREE_VIEW (item), column);

        gtk_tree_view_set_model( GTK_TREE_VIEW(item), GTK_TREE_MODEL(list));

        
        PRIVATE(ngl)->players_list = list;
        
        PRIVATE(ngl)->game = g_malloc(sizeof(NetworkGame));

        PRIVATE(ngl)->game->clients = NULL;
        return ngl;
        
}


struct StatusJob {
        NetworkGameLauncher * launcher;
        gchar * message;
} StatusJob;


gboolean status_idle(gpointer data) {
        struct StatusJob * j;
        NetworkGameLauncher  * launcher;
        
        j = (struct StatusJob *) data;
        
        launcher = j->launcher;
        
        gtk_label_set_label(PRIVATE(launcher)->connection_label,
                            j->message);
        
        g_free(j->message);
        g_free(j);
        return FALSE;
}



static void set_status_message(NetworkGameLauncher * launcher,
                               const gchar * message) {

        struct StatusJob * j;
        j = (struct StatusJob *)g_malloc( sizeof(StatusJob));
        j->launcher = launcher;
        j->message = g_strdup(message);
        g_idle_add(status_idle,j);
}

static void connect_server_signal(gpointer    callback_data,
                           guint       callback_action,
                           GtkWidget  *widget) {
        NetworkGameLauncher  * launcher;
        GtkWidget * entry;

        launcher = NETWORK_GAME_LAUNCHER(callback_data);

        entry =  glade_xml_get_widget( PRIVATE(launcher)->glade_xml, "server_name_entry");
        if( PRIVATE(launcher)->server_name != NULL) {
                g_free(PRIVATE(launcher)->server_name);
        }

        PRIVATE(launcher)->server_name = 
                g_strdup( gtk_entry_get_text(GTK_ENTRY(entry)));

        
        set_status_message(launcher,  g_strdup_printf("Connecting %s ...",
                                                 PRIVATE(launcher)->server_name));


        set_sensitive( glade_xml_get_widget( PRIVATE(launcher)->glade_xml
                                             , "connect_hbox"),FALSE);        
        connect_server(launcher);
}

void free_network_client(gpointer data,gpointer user_data) {

        NetworkClient * c = (NetworkClient *) data;
        g_free(c->client_name);
        g_free(c);

}


static void quit_server_signal(gpointer    callback_data,
                           guint       callback_action,
                               GtkWidget  *widget) {


        NetworkGameLauncher * launcher;

        launcher = NETWORK_GAME_LAUNCHER(callback_data);

        g_list_foreach(PRIVATE(launcher)->game->clients,
                       free_network_client,
                       NULL);
        g_list_free( PRIVATE(launcher)->game->clients);

        PRIVATE(launcher)->game->clients = NULL;
        
        update_players_list(launcher);
        
        monkey_message_handler_send_message( PRIVATE(launcher)->handler,
                                             PRIVATE(launcher)->client_id,
                                             "DISCONNECT",strlen("DISCONNECT"));

        monkey_message_handler_disconnect(PRIVATE(launcher)->handler);
        
        monkey_message_handler_join(PRIVATE(launcher)->handler);
        g_object_unref( PRIVATE(launcher)->handler);
        PRIVATE(launcher)->handler = NULL;
               set_sensitive( glade_xml_get_widget( PRIVATE(launcher)->glade_xml
                                                    , "connected_game_hbox"),FALSE);
        
               set_sensitive( glade_xml_get_widget( PRIVATE(launcher)->glade_xml
                                                    , "connect_hbox"),TRUE);        
}




void recv_network_init_message(MonkeyMessageHandler * mmh,
		  guint32 client_id,
		  gchar * message,
		  gpointer * p) {

        NetworkGameLauncher * launcher;
        gchar * m;

        launcher = NETWORK_GAME_LAUNCHER(p);

        if( g_str_equal(message,"INIT")) {
                g_log(G_LOG_DOMAIN,G_LOG_LEVEL_DEBUG,"message recieved client_id %d, message %s\n",client_id,message);
                
                
                m = g_strdup_printf("INIT_MESSAGE\nname=%s",g_get_user_name());
                

                g_signal_connect( G_OBJECT(PRIVATE(launcher)->handler),
                                  "recv-xml-message",
                                  G_CALLBACK( recv_network_xml_message ),
                                  launcher);

                PRIVATE(launcher)->client_id = client_id;
                monkey_message_handler_send_message(PRIVATE(launcher)->handler,
                                                    client_id,
                                                    m,
                                                    strlen(m));
                set_status_message(launcher, g_strdup_printf("Connected to %s",
                                                        PRIVATE(launcher)->server_name));

                set_sensitive( glade_xml_get_widget( PRIVATE(launcher)->glade_xml
                                                     , "connected_game_hbox"),TRUE); 

        } else if( g_str_has_prefix(message,"INIT_NOT_OK") ) {
                set_sensitive( glade_xml_get_widget( PRIVATE(launcher)->glade_xml
                                                     , "connect_hbox"),TRUE); 

                g_print("init not ok!!");
                set_status_message(launcher,"Not ok");
        }
                                            
}



gboolean update_players_list_idle(gpointer data) {
        NetworkGameLauncher * launcher;

        GList * next;

        launcher = NETWORK_GAME_LAUNCHER(data);
        gtk_list_store_clear( PRIVATE( launcher)->players_list);
        
        if( PRIVATE(launcher)->game != NULL) {
                next = PRIVATE(launcher)->game->clients;
                while(next != NULL) {
                        NetworkClient * nc;
                        GtkTreeIter iter;
                        GtkListStore * tree;
                        
                        nc = (NetworkClient *)next->data;

                        
                        tree =  PRIVATE(launcher)->players_list;
                        
                        gtk_list_store_append (tree, &iter);
                        gtk_list_store_set (tree, &iter,
                                            0,nc->client_name,
                                            1,nc->game_owner,
                                            -1);
                        
                        next = g_list_next(next);
                        
                }
        }
        return FALSE;

}

static void update_players_list(NetworkGameLauncher * launcher) {
        g_idle_add(update_players_list_idle,(gpointer)launcher);
        
}

void recv_network_xml_message(MonkeyMessageHandler * mmh,
		  guint32 client_id,
		  xmlDoc * message,
		  gpointer * p) {

        xmlNode * root;
        xmlNode * current;
        NetworkGameLauncher * launcher;
        NetworkGame * game;
        
        root = message->children;
        
        current = root->children;

        launcher = NETWORK_GAME_LAUNCHER(p);
        game = PRIVATE(launcher)->game;


        if( game->clients != NULL) {
                g_list_foreach(game->clients,free_network_client,NULL);
                
                g_list_free(game->clients);
                game->clients = NULL;
        }
        
        
        
        while(current != NULL) {
                gboolean owner;
                xmlChar * prop;
                NetworkClient * nc;
                prop = xmlGetProp(current,"owner");
                
                if( prop != NULL && strcmp( prop,"true")  == 0) {
                        owner = TRUE;
                } else {
                        owner = FALSE;
                }
                
                nc = g_malloc(sizeof(NetworkClient));
                nc->client_name = g_strdup(current->children->content);
                nc->game_owner = owner; 
                game->clients = g_list_append( game->clients,
                                               nc);
                current = current->next;
                
        }

        
        update_players_list(launcher);
}

gboolean set_sensitive_true_idle(gpointer data) {
        gtk_widget_set_sensitive( GTK_WIDGET(data),
                                  TRUE);

        return FALSE;

}

gboolean set_sensitive_false_idle(gpointer data) {
        gtk_widget_set_sensitive( GTK_WIDGET(data),
                                  FALSE);

        return FALSE;

}

static void set_sensitive(GtkWidget * w,
                          gboolean s) {
        if( s) {
                g_idle_add(set_sensitive_true_idle,w);
        } else {
                g_idle_add(set_sensitive_false_idle,w);
        }
 }


static gboolean connect_server(NetworkGameLauncher * launcher) {
        
        int sock;
        struct sockaddr_in sock_client;
        struct hostent *src_host;
        
        g_print("connect sever \n");
        sock =  socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  
 
        if ( sock == -1) {
                perror("socket()");
                return FALSE;
        }
        
        
        bzero((char *) &sock_client, sizeof(sock_client));
        sock_client.sin_family = AF_INET;
        sock_client.sin_port = (unsigned short) htons(6666);
        src_host = (struct hostent *) gethostbyname(PRIVATE(launcher)->server_name);	
        if (!src_host) {
                fprintf(stderr, "Not a valid Server IP...\n");
                return FALSE;
        }
        
        bcopy( (char *) src_host->h_addr, (char *) &sock_client.sin_addr.s_addr, src_host->h_length);
        
        while (connect(sock, (struct sockaddr *) &sock_client, sizeof(sock_client)) == -1) {
                if (errno != EAGAIN)
                        {
                                perror("connect()");
                                return FALSE;
                        }
        }

        PRIVATE(launcher)->handler = monkey_message_handler_new(sock);
        
   
        g_signal_connect( G_OBJECT(PRIVATE(launcher)->handler),
                          "recv-message",
                          G_CALLBACK( recv_network_init_message ),
                          launcher);

        monkey_message_handler_start_listening(PRIVATE(launcher)->handler);
        return FALSE;
}


void network_game_launcher_finalize(GObject *object) {
        NetworkGameLauncher * launcher = (NetworkGameLauncher *) object;
	
        g_free(PRIVATE(launcher));

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }		
}

static void network_game_launcher_instance_init(NetworkGameLauncher * launcher) {
        launcher->private =g_new0 (NetworkGameLauncherPrivate, 1);
}

static void network_game_launcher_class_init (NetworkGameLauncherClass *klass) {
        GObjectClass* object_class;
        
        parent_class = g_type_class_peek_parent(klass);
        object_class = G_OBJECT_CLASS(klass);
        object_class->finalize = network_game_launcher_finalize;
}

GType network_game_launcher_get_type(void) {
        static GType network_game_launcher_type = 0;
        
        if (!network_game_launcher_type) {
                static const GTypeInfo network_game_launcher_info = {
                        sizeof(NetworkGameLauncherClass),
                        NULL,           /* base_init */
                        NULL,           /* base_finalize */
                        (GClassInitFunc) network_game_launcher_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof(NetworkGameLauncher),
                        1,              /* n_preallocs */
                        (GInstanceInitFunc) network_game_launcher_instance_init,
                };
                
                network_game_launcher_type = g_type_register_static(G_TYPE_OBJECT,
                                                                     "NetworkGameLauncher",
                                                                     &network_game_launcher_info, 0
                                                                     );

        }
        
        return network_game_launcher_type;
}
