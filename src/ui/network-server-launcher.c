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

#include "message-handler.h"

#include "simple-server.h"
#include "network-server-launcher.h"
#include "game-network-player-manager.h"
#include "ui-main.h"
#include "game-manager-proxy.h"

struct NetworkServerLauncherPrivate {
        GladeXML * glade_xml;
        GtkWidget * window;
        gchar * server_name;
        NetworkMessageHandler * handler;
        int client_id;
        gboolean ready;
        GtkLabel * connection_label;
        GtkListStore * players_list;
        NetworkSimpleServer * manager;
        NetGameManagerProxy * manager_proxy;
        gboolean np_from_server;
        gboolean ng_from_server;
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


static void number_of_players_changed(NetworkServerLauncher * launcher,
                                      GtkWidget  *widget);


static void number_of_games_changed(NetworkServerLauncher * launcher,
                                      GtkWidget  *widget);


static void start_signal(gpointer    callback_data,
                         guint       callback_action,
                         GtkWidget  *widget);

static gboolean connect_server(NetworkServerLauncher * launcher);


static void set_sensitive(GtkWidget * w,
                          gboolean s);

static void update_players_list(NetworkServerLauncher * launcher);

static void recv_network_xml_message(NetworkMessageHandler * mmh,
                              guint32 client_id,
                              xmlDoc * message,
                              gpointer * p);

static void set_number_of_games(NetworkServerLauncher * launcher,
                                int n);

static void set_number_of_players(NetworkServerLauncher * launcher,
                                  int n);
 
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

        item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml,"number_of_players");
        g_signal_connect_swapped( item, "value_changed", GTK_SIGNAL_FUNC(number_of_players_changed),
                                  ngl);


        item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml,"number_of_games");
        g_signal_connect_swapped( item, "value_changed", GTK_SIGNAL_FUNC(number_of_games_changed),
                                  ngl);
        
        PRIVATE(ngl)->players_list = list;
        
        PRIVATE(ngl)->server_name = "localhost";

        PRIVATE(ngl)->manager = network_simple_server_new();

        network_simple_server_start(PRIVATE(ngl)->manager);

        connect_server(ngl);

        return ngl;
        
}


struct StatusJob {
        NetworkServerLauncher * launcher;
        gchar * message;
} StatusJob;



static void 
number_of_players_changed(NetworkServerLauncher * launcher,
                          GtkWidget  *widget)
{

        if( PRIVATE(launcher)->np_from_server == FALSE ) {
                net_game_manager_proxy_send_number_of_players(PRIVATE(launcher)->manager_proxy,
                                                              gtk_spin_button_get_value_as_int ( GTK_SPIN_BUTTON(widget)));
        }

        
}



static void 
number_of_games_changed(NetworkServerLauncher * launcher,
                          GtkWidget  *widget)
{

        if( PRIVATE(launcher)->ng_from_server == FALSE) {
                net_game_manager_proxy_send_number_of_games(PRIVATE(launcher)->manager_proxy,
                                                            gtk_spin_button_get_value_as_int ( GTK_SPIN_BUTTON(widget)));
        }

        
}





static void send_disconnect(NetworkServerLauncher * launcher) {
        xmlDoc * doc;
        xmlNode * root;
        
        doc = xmlNewDoc("1.0");
        root = xmlNewNode(NULL,
                          "message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,"name","disconnect");
        
        network_message_handler_send_xml_message(PRIVATE(launcher)->handler,
                                                PRIVATE(launcher)->client_id,
                                                doc);

        xmlFreeDoc(doc);

}

static void quit_server_signal(gpointer    callback_data,
                           guint       callback_action,
                               GtkWidget  *widget) {


        NetworkServerLauncher * launcher;

        launcher = NETWORK_SERVER_LAUNCHER(callback_data);


        update_players_list(launcher);

        if( PRIVATE(launcher)->handler != NULL) {
                send_disconnect(launcher);
                
                network_message_handler_disconnect(PRIVATE(launcher)->handler);
        
                g_object_unref( PRIVATE(launcher)->handler);
                PRIVATE(launcher)->handler = NULL;

                network_simple_server_stop(PRIVATE(launcher)->manager);
        }

        
        gtk_widget_destroy( PRIVATE(launcher)->window);


}



static void ready_signal(gpointer    callback_data,
                         guint       callback_action,
                         GtkWidget  *widget) {


        NetworkServerLauncher * launcher;
        GtkWidget * item;

        launcher = NETWORK_SERVER_LAUNCHER(callback_data);

        item = glade_xml_get_widget( PRIVATE(launcher)->glade_xml, "ready_button");

        if( ! PRIVATE(launcher)->ready ) {
                net_game_manager_proxy_send_ready_state(PRIVATE(launcher)->manager_proxy,
                                                        TRUE);
                PRIVATE(launcher)->ready = TRUE;
        } else {

                net_game_manager_proxy_send_ready_state(PRIVATE(launcher)->manager_proxy,
                                                        FALSE);
                
                PRIVATE(launcher)->ready = FALSE;
                
        }
}



static void start_signal(gpointer    callback_data,
                         guint       callback_action,
                         GtkWidget  *widget) {


        NetworkServerLauncher * launcher;

        launcher = NETWORK_SERVER_LAUNCHER(callback_data);

        net_game_manager_proxy_send_start( PRIVATE(launcher)->manager_proxy);

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

        network_message_handler_send_xml_message(PRIVATE(launcher)->handler,
                                                PRIVATE(launcher)->client_id,
                                                doc);

        xmlFreeDoc(doc);

}





static gboolean update_players_list_idle(gpointer data) {

        NetworkServerLauncher * launcher;

        GList * next;

        launcher = NETWORK_SERVER_LAUNCHER(data);
        gtk_list_store_clear( PRIVATE( launcher)->players_list);
        
        if( PRIVATE(launcher)->manager_proxy != NULL) {
                next = net_game_manager_proxy_get_players( PRIVATE(launcher)->manager_proxy);

                
                while(next != NULL) {
                        GtkTreeIter iter;
                        GtkListStore * tree;
                        Client * client;
                        client = (Client *)next->data;

                        
                        tree =  PRIVATE(launcher)->players_list;
                        
                        gtk_list_store_append (tree, &iter);
                        gtk_list_store_set (tree, &iter,
                                            0,client->name,
                                            1,client->owner,
                                            2,client->ready,
                                            -1);
                        
                        next = g_list_next(next);
                        
                }
        }
        return FALSE;

}

static void update_players_list(NetworkServerLauncher * launcher) {
        g_idle_add(update_players_list_idle,(gpointer)launcher);
        
}



static gboolean start_game_idle(gpointer data) {
        NetworkServerLauncher * launcher;


        launcher = NETWORK_SERVER_LAUNCHER(data);

        g_print("network-server-laucnehr destroy widget\n");
        gtk_widget_destroy( PRIVATE(launcher)->window);

        g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(launcher)->handler ),
                                               G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,launcher);
                      

        g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(launcher)->manager_proxy ),
                                               G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,launcher);
                      
        g_print("Network-server-launcher : game started\n");


        return FALSE;
}

static void 
players_list_updated(NetGameManagerProxy * proxy,
                     NetworkServerLauncher * launcher) 
{

        update_players_list(launcher);
}






static gboolean
update_number_of_players_idle(gpointer data) 
{
        NetworkServerLauncher * launcher;
        GtkWidget * item;

        launcher = NETWORK_SERVER_LAUNCHER(data);
        item = glade_xml_get_widget( PRIVATE(launcher)->glade_xml, "number_of_players");
        PRIVATE(launcher)->np_from_server = TRUE;
        gtk_spin_button_set_value ( GTK_SPIN_BUTTON(item), 
                                    net_game_manager_proxy_get_number_of_players(PRIVATE(launcher)->manager_proxy));

        PRIVATE(launcher)->np_from_server = FALSE;
        return FALSE;
}

static void
update_number_of_players(NetworkServerLauncher * launcher)
{
        g_idle_add(update_number_of_players_idle,launcher);
        
}

static void 
net_number_of_players_changed(NetGameManagerProxy * proxy,
                          NetworkServerLauncher * launcher) 
{
        update_number_of_players(launcher);
}


static gboolean
update_number_of_games_idle(gpointer data) 
{
        NetworkServerLauncher * launcher;
        GtkWidget * item;

        launcher = NETWORK_SERVER_LAUNCHER(data);
        item = glade_xml_get_widget( PRIVATE(launcher)->glade_xml, "number_of_games");
        PRIVATE(launcher)->ng_from_server = TRUE;
        gtk_spin_button_set_value ( GTK_SPIN_BUTTON(item), 
                                    net_game_manager_proxy_get_number_of_games(PRIVATE(launcher)->manager_proxy));

        PRIVATE(launcher)->ng_from_server = FALSE;
        return FALSE;
}

static void
update_number_of_games(NetworkServerLauncher * launcher)
{
        g_idle_add(update_number_of_games_idle,launcher);
        
}

static void 
net_number_of_games_changed(NetGameManagerProxy * proxy,
                          NetworkServerLauncher * launcher) 
{
        update_number_of_games(launcher);
}


static void 
game_created(NetGameManagerProxy * proxy,
             NetworkServerLauncher * launcher) 
{
        start_game(launcher);
}

static void start_game(NetworkServerLauncher * launcher) {
        g_idle_add(start_game_idle,launcher);
        
}

static void recv_network_xml_message(NetworkMessageHandler * mmh,
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
                
                
        } else if( g_str_equal(message_name,"init_request") ) {
                
                
                PRIVATE(launcher)->client_id = client_id;
                
                
                
                send_init(launcher);
                
                
                
                
        } else if(g_str_equal(message_name,"init_reply") ){
                          
                if( g_str_equal(root->children->content,"ok")) {
                        g_print("network-server-launcher : init ok\n");
                        PRIVATE(launcher)->manager_proxy =
                                net_game_manager_proxy_new( PRIVATE(launcher)->handler,client_id);

                        g_signal_connect( PRIVATE(launcher)->manager_proxy,
                                          "players-list-updated",
                                          G_CALLBACK(players_list_updated),
                                          launcher);

                        g_signal_connect( PRIVATE(launcher)->manager_proxy,
                                          "number-of-players-changed",
                                          G_CALLBACK(net_number_of_players_changed),
                                          launcher);

                        g_signal_connect( PRIVATE(launcher)->manager_proxy,
                                          "number-of-games-changed",
                                          G_CALLBACK(net_number_of_games_changed),
                                          launcher);


                        g_signal_connect( PRIVATE(launcher)->manager_proxy,
                                          "game-created",
                                          G_CALLBACK(game_created),
                                          launcher);

                } else {
                        g_print("network-server-launcer : init not ok\n");
                        
                }
        } else if(g_str_equal(message_name,"game_joined")) {
                int game_id;
                GameNetworkPlayerManager * manager;
                UiMain * ui;

                sscanf(root->children->content,"%d",&game_id);
             
                
                ui = ui_main_get_instance();
                manager = game_network_player_manager_new(ui_main_get_window(ui),
                                                          ui_main_get_canvas(ui),
                                                          PRIVATE(launcher)->handler,
                                                          PRIVATE(launcher)->client_id);
                ui_main_set_game_manager(ui,
                                         GAME_MANAGER(manager));

        } else if( g_str_equal(message_name,"number_of_games")) {
		int number;
            
		sscanf(root->children->content,"%d",&number);
                set_number_of_games(launcher,number);
	} else if( g_str_equal(message_name,"number_of_players")) {
		int number;
            
		sscanf(root->children->content,"%d",&number);
                set_number_of_players(launcher,number);
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



static void
set_number_of_games(NetworkServerLauncher * launcher,
                    int n) 
{
}

static void 
set_number_of_players(NetworkServerLauncher * launcher,
                      int n) 
{
        g_print("number of players %d\n",n);
}


static gboolean connect_server(NetworkServerLauncher * launcher) {

        PRIVATE(launcher)->handler = network_message_handler_new(0);
        if( ! network_message_handler_connect( PRIVATE(launcher)->handler,
                                              PRIVATE(launcher)->server_name,
                                              6666)  ) {
        
                g_object_unref( G_OBJECT(PRIVATE(launcher)->handler));
                PRIVATE(launcher)->handler = NULL;
                return FALSE;
        }
   

        g_signal_connect( G_OBJECT(PRIVATE(launcher)->handler),
                          "recv-xml-message",
                          G_CALLBACK( recv_network_xml_message ),
                          launcher);


        network_message_handler_start_listening(PRIVATE(launcher)->handler);

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
        PRIVATE(launcher)->np_from_server = FALSE;
        PRIVATE(launcher)->ng_from_server = FALSE;
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
