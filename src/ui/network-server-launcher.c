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
#include <glade/glade.h>

#include "monkey-message-handler.h"

#include "network-server-launcher.h"
#include "game-network-player-manager.h"
#include "ui-main.h"
#include "mn-game-manager.h"

typedef struct NetworkClient {
        gchar *        client_name;
        gboolean       game_owner;
        gboolean       ready;
} NetworkClient;


typedef struct NetworkGame {
        GList *               clients;
} NetworkGame;

struct NetworkServerLauncherPrivate {
        GladeXML * glade_xml;
        GtkWidget * window;
        gchar * server_name;
        MonkeyMessageHandler * handler;
        int client_id;
        gboolean ready;
        GtkLabel * connection_label;
        GtkListStore * players_list;
        NetworkGame * game;
        MnGameManager * manager;
};



#define PRIVATE( NetworkServerLauncher ) (NetworkServerLauncher->private)

static GObjectClass* parent_class = NULL;

void network_server_launcher_finalize(GObject *);

static void start_game(NetworkServerLauncher * launcher);



static void quit_server_signal(gpointer    callback_data,
                           guint       callback_action,
                           GtkWidget  *widget);

static void ready_signal(gpointer    callback_data,
                         guint       callback_action,
                         GtkWidget  *widget);


static void start_signal(gpointer    callback_data,
                         guint       callback_action,
                         GtkWidget  *widget);

static gboolean connect_server(NetworkServerLauncher * launcher);


static void set_sensitive(GtkWidget * w,
                          gboolean s);

static void update_players_list(NetworkServerLauncher * launcher);

static void recv_network_xml_message(MonkeyMessageHandler * mmh,
                              guint32 client_id,
                              xmlDoc * message,
                              gpointer * p);

NetworkServerLauncher *network_server_launcher_new() {
        NetworkServerLauncher * ngl;
        GtkWidget * item;

        GtkTreeViewColumn * column;
        GtkListStore * list;

        ngl = 
                NETWORK_SERVER_LAUNCHER(g_object_new(TYPE_NETWORK_SERVER_LAUNCHER
                                                    , NULL));

        PRIVATE(ngl)->server_name = NULL;

        PRIVATE(ngl)->ready = FALSE;

        PRIVATE(ngl)->glade_xml = glade_xml_new(DATADIR"/monkey-bubble/glade/netserver.glade","network_window",NULL);
        
        PRIVATE(ngl)->window = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "network_window");

       


        item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "quit_button");
        g_signal_connect_swapped( item,"clicked",GTK_SIGNAL_FUNC(quit_server_signal),ngl);


        item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "ready_button");
        g_signal_connect_swapped( item,"clicked",GTK_SIGNAL_FUNC(ready_signal),ngl);
        
        item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "start_button");
        g_signal_connect_swapped( item,"clicked",GTK_SIGNAL_FUNC(start_signal),ngl);
        


       item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml,"players_treeview");
       list = gtk_list_store_new(3,G_TYPE_STRING,G_TYPE_BOOLEAN,G_TYPE_BOOLEAN);

        column = gtk_tree_view_column_new_with_attributes("Player name",gtk_cell_renderer_text_new(),
                                                          "text",0);

        gtk_tree_view_append_column (GTK_TREE_VIEW (item), column);

        column = gtk_tree_view_column_new();

        column = gtk_tree_view_column_new_with_attributes("Owner",gtk_cell_renderer_toggle_new(),
                                                          "active",1);

        gtk_tree_view_append_column (GTK_TREE_VIEW (item), column);

        column = gtk_tree_view_column_new();

        column = gtk_tree_view_column_new_with_attributes("Ready",gtk_cell_renderer_toggle_new(),
                                                          "active",2);

        gtk_tree_view_append_column (GTK_TREE_VIEW (item), column);


        gtk_tree_view_set_model( GTK_TREE_VIEW(item), GTK_TREE_MODEL(list));

        
        PRIVATE(ngl)->players_list = list;
        
        PRIVATE(ngl)->game = g_malloc(sizeof(NetworkGame));

        PRIVATE(ngl)->game->clients = NULL;

        PRIVATE(ngl)->server_name = "localhost";

        PRIVATE(ngl)->manager = mn_game_manager_new();

        mn_game_manager_start_server(PRIVATE(ngl)->manager);

        connect_server(ngl);

        return ngl;
        
}


struct StatusJob {
        NetworkServerLauncher * launcher;
        gchar * message;
} StatusJob;



static void free_network_client(gpointer data,gpointer user_data) {

        NetworkClient * c = (NetworkClient *) data;
        g_free(c->client_name);
        g_free(c);

}


static void send_disconnect(NetworkServerLauncher * launcher) {
        xmlDoc * doc;
        xmlNode * root;
        
        doc = xmlNewDoc("1.0");
        root = xmlNewNode(NULL,
                          "message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,"name","disconnect");
        
        monkey_message_handler_send_xml_message(PRIVATE(launcher)->handler,
                                                PRIVATE(launcher)->client_id,
                                                doc);

        xmlFreeDoc(doc);

}

static void quit_server_signal(gpointer    callback_data,
                           guint       callback_action,
                               GtkWidget  *widget) {


        NetworkServerLauncher * launcher;

        launcher = NETWORK_SERVER_LAUNCHER(callback_data);

        g_list_foreach(PRIVATE(launcher)->game->clients,
                       free_network_client,
                       NULL);
        g_list_free( PRIVATE(launcher)->game->clients);

        PRIVATE(launcher)->game->clients = NULL;
        
        update_players_list(launcher);
        
        send_disconnect(launcher);

        monkey_message_handler_disconnect(PRIVATE(launcher)->handler);
        
        monkey_message_handler_join(PRIVATE(launcher)->handler);
        g_object_unref( PRIVATE(launcher)->handler);
        PRIVATE(launcher)->handler = NULL;

        mn_game_manager_stop_server(PRIVATE(launcher)->manager);
        gtk_widget_destroy( PRIVATE(launcher)->window);


}


static void send_ready_state(NetworkServerLauncher * launcher,
                             gboolean state) {
        xmlDoc * doc;
        xmlNode * root;
        xmlNode * text;
        
        doc = xmlNewDoc("1.0");
        root = xmlNewNode(NULL,
                          "message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,"name","ready_state");
        
        text = xmlNewText((state ? "true" : "false"));
        
        xmlAddChild(root,text);

        monkey_message_handler_send_xml_message(PRIVATE(launcher)->handler,
                                                PRIVATE(launcher)->client_id,
                                                doc);

        xmlFreeDoc(doc);

}

static void ready_signal(gpointer    callback_data,
                         guint       callback_action,
                         GtkWidget  *widget) {


        NetworkServerLauncher * launcher;
        GtkWidget * item;

        launcher = NETWORK_SERVER_LAUNCHER(callback_data);

        item = glade_xml_get_widget( PRIVATE(launcher)->glade_xml, "ready_button");

        if( ! PRIVATE(launcher)->ready ) {
                send_ready_state(launcher,TRUE);
                gtk_button_set_label( GTK_BUTTON(item),"not Ready");
                PRIVATE(launcher)->ready = TRUE;
        } else {
                send_ready_state(launcher,FALSE);
                gtk_button_set_label( GTK_BUTTON(item),"Ready");
                
                PRIVATE(launcher)->ready = FALSE;
                
        }
}

static void send_start(NetworkServerLauncher * launcher) {
        xmlDoc * doc;
        xmlNode * root;
        xmlNode * text;
        
        doc = xmlNewDoc("1.0");
        root = xmlNewNode(NULL,
                          "message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,"name","start_game");
        
        text = xmlNewText("1"); // game_id
        
        xmlAddChild(root,text);

        monkey_message_handler_send_xml_message(PRIVATE(launcher)->handler,
                                                PRIVATE(launcher)->client_id,
                                                doc);

        xmlFreeDoc(doc);

}


static void start_signal(gpointer    callback_data,
                         guint       callback_action,
                         GtkWidget  *widget) {


        NetworkServerLauncher * launcher;

        launcher = NETWORK_SERVER_LAUNCHER(callback_data);

        send_start(launcher);

        set_sensitive( glade_xml_get_widget( PRIVATE(launcher)->glade_xml
                                             , "start_button"),FALSE); 

        set_sensitive( glade_xml_get_widget( PRIVATE(launcher)->glade_xml
                                             , "quit_button"),FALSE); 

        set_sensitive( glade_xml_get_widget( PRIVATE(launcher)->glade_xml
                                             , "ready_button"),FALSE); 

}


static void send_init(NetworkServerLauncher * launcher) {
        xmlDoc * doc;
        xmlNode * current, * root;
        xmlNode * text;
        
        doc = xmlNewDoc("1.0");
        root = xmlNewNode(NULL,
                          "message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,"name","init");
        
        current = xmlNewNode(NULL,
                             "player");
        
        text = xmlNewText(g_get_user_name());
        
        xmlAddChild(current,text);
        xmlAddChild(root,current);

        monkey_message_handler_send_xml_message(PRIVATE(launcher)->handler,
                                                PRIVATE(launcher)->client_id,
                                                doc);

        xmlFreeDoc(doc);

}





static gboolean update_players_list_idle(gpointer data) {

        NetworkServerLauncher * launcher;

        GList * next;

        launcher = NETWORK_SERVER_LAUNCHER(data);
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
                                            2,nc->ready,
                                            -1);
                        
                        next = g_list_next(next);
                        
                }
        }
        return FALSE;

}

static void update_players_list(NetworkServerLauncher * launcher) {
        g_idle_add(update_players_list_idle,(gpointer)launcher);
        
}

static void recv_player_list(NetworkServerLauncher * launcher,xmlDoc * doc) {

        xmlNode * root;
        xmlNode * current;
        NetworkGame * game;

        root = doc->children;

        current = root->children;


        game = PRIVATE(launcher)->game;
        
        
        if( game->clients != NULL) {
                g_list_foreach(game->clients,free_network_client,NULL);
                
                g_list_free(game->clients);
                game->clients = NULL;
        }
        
        
        
        while(current != NULL) {
                gboolean owner;
                gboolean ready;
                xmlChar * prop;
                NetworkClient * nc;
                prop = xmlGetProp(current,"owner");
                
                if( prop != NULL && strcmp( prop,"true")  == 0) {
                        owner = TRUE;
                } else {
                        owner = FALSE;
                }
                
                prop = xmlGetProp(current,"ready");                
                if( prop != NULL && strcmp( prop,"true")  == 0) {
                        ready = TRUE;
                } else {
                        ready = FALSE;
                }
                
                nc = g_malloc(sizeof(NetworkClient));
                nc->client_name = g_strdup(current->children->content);
                nc->game_owner = owner; 
                nc->ready = ready;
                game->clients = g_list_append( game->clients,
                                               nc);
                current = current->next;
                            
        }

        update_players_list(launcher);
        
}

static gboolean start_game_idle(gpointer data) {
        NetworkServerLauncher * launcher;
        GameNetworkPlayerManager * manager;
        UiMain * ui;

        launcher = NETWORK_SERVER_LAUNCHER(data);

        gtk_widget_destroy( PRIVATE(launcher)->window);
        ui = ui_main_get_instance();
        manager = game_network_player_manager_new(ui_main_get_window(ui),
                                                  ui_main_get_canvas(ui),
                                                  PRIVATE(launcher)->handler,
                                                  PRIVATE(launcher)->client_id);
        ui_main_set_game_manager(ui,
                                 GAME_MANAGER(manager));

        g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(launcher)->handler ),
                                               G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,launcher);
                      
        return FALSE;
}

static void start_game(NetworkServerLauncher * launcher) {
        g_idle_add(start_game_idle,launcher);
        
}

static void recv_network_xml_message(MonkeyMessageHandler * mmh,
		  guint32 client_id,
		  xmlDoc * message,
		  gpointer * p) {

        xmlNode * root;
        NetworkServerLauncher * launcher;
        char * message_name;
        
        root = message->children;
        
        g_assert( g_str_equal(root->name,"message"));
        
        launcher = NETWORK_SERVER_LAUNCHER(p);
        
        message_name = xmlGetProp(root,"name");
        
        if( g_str_equal(message_name,"game_player_list") ) {
                
                recv_player_list(launcher,message);
                
        } else if( g_str_equal(message_name,"init_request") ) {
                
                g_print("INIT REQUIEST");
                
                PRIVATE(launcher)->client_id = client_id;
                
                
                
                send_init(launcher);
                
                
                
                
        } else if(g_str_equal(message_name,"init_reply") ){
                          
                if( g_str_equal(root->children->content,"ok")) {
                        g_print("init ok!!");
                        
                } else {
                        g_print("init not ok!!");
                        
                }
        } else if(g_str_equal(message_name,"game_joined")) {
                int game_id;
                
                sscanf(root->children->content,"%d",&game_id);
             
        } else if(g_str_equal(message_name,"game_started") ) {
                
                int game_id;
                
                sscanf(root->children->content,"%d",&game_id);
                g_print("game started %d \n",game_id);                

                start_game(launcher);

        }

        
        
        
}

static gboolean set_sensitive_true_idle(gpointer data) {
        gtk_widget_set_sensitive( GTK_WIDGET(data),
                                  TRUE);

        return FALSE;

}

static gboolean set_sensitive_false_idle(gpointer data) {
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


static gboolean connect_server(NetworkServerLauncher * launcher) {
        
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
                          "recv-xml-message",
                          G_CALLBACK( recv_network_xml_message ),
                          launcher);

        monkey_message_handler_start_listening(PRIVATE(launcher)->handler);

        PRIVATE(launcher)->ready = FALSE;
        return TRUE;
}


void network_server_launcher_finalize(GObject *object) {
        NetworkServerLauncher * launcher = (NetworkServerLauncher *) object;
	
        g_free(PRIVATE(launcher));

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }		
}

static void network_server_launcher_instance_init(NetworkServerLauncher * launcher) {
        launcher->private =g_new0 (NetworkServerLauncherPrivate, 1);
}

static void network_server_launcher_class_init (NetworkServerLauncherClass *klass) {
        GObjectClass* object_class;
        
        parent_class = g_type_class_peek_parent(klass);
        object_class = G_OBJECT_CLASS(klass);
        object_class->finalize = network_server_launcher_finalize;
}

GType network_server_launcher_get_type(void) {
        static GType network_server_launcher_type = 0;
        
        if (!network_server_launcher_type) {
                static const GTypeInfo network_server_launcher_info = {
                        sizeof(NetworkServerLauncherClass),
                        NULL,           /* base_init */
                        NULL,           /* base_finalize */
                        (GClassInitFunc) network_server_launcher_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof(NetworkServerLauncher),
                        1,              /* n_preallocs */
                        (GInstanceInitFunc) network_server_launcher_instance_init,
                };
                
                network_server_launcher_type = g_type_register_static(G_TYPE_OBJECT,
                                                                     "NetworkServerLauncher",
                                                                     &network_server_launcher_info, 0
                                                                     );

        }
        
        return network_server_launcher_type;
}
