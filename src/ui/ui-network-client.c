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
#include <config.h>
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
#include <glib/gi18n.h>

#include "message-handler.h"

#include "ui-network-client.h"
#include "game-network-player-manager.h"
#include "ui-main.h"
#include "game-manager-proxy.h"



struct UiNetworkClientPrivate {
        GladeXML * glade_xml;
        GtkWidget * window;
        gchar * server_name;
        NetworkMessageHandler * handler;
        int client_id;
        gboolean ready;
        GtkLabel * connection_label;
        GtkListStore * players_list;
        NetGameManagerProxy * manager_proxy;

};



#define PRIVATE( UiNetworkClient ) (UiNetworkClient->private)

static GObjectClass* parent_class = NULL;

void ui_network_client_finalize(GObject *);


static void start_game(UiNetworkClient * self);

static void connect_server_signal(gpointer    callback_data,
                           guint       callback_action,
                           GtkWidget  *widget);


static gboolean quit_server_signal(gpointer    callback_data,
                           guint       callback_action,
                           GtkWidget  *widget);
static gboolean quit_signal(gpointer    callback_data,
                           guint       callback_action,
                           GtkWidget  *widget);



static void ready_signal(gpointer    callback_data,
                         guint       callback_action,
                         GtkWidget  *widget);


static gboolean connect_server(UiNetworkClient * self);

static void set_status_message(UiNetworkClient * self,
                               const gchar * message);

static void set_sensitive(GtkWidget * w,
                          gboolean s);

static void update_players_list(UiNetworkClient * self);

void recv_network_xml_message(NetworkMessageHandler * mmh,
                              guint32 client_id,
                              xmlDoc * message,
                              gpointer * p);

UiNetworkClient *ui_network_client_new() {
        UiNetworkClient * ngl;
        GtkWidget * item;

        GtkTreeViewColumn * column;
        GtkListStore * list;

        ngl = 
                UI_NETWORK_CLIENT(g_object_new(TYPE_UI_NETWORK_CLIENT
                                                    , NULL));

        PRIVATE(ngl)->server_name = NULL;

        PRIVATE(ngl)->ready = FALSE;

#ifdef GNOME
        PRIVATE(ngl)->glade_xml = glade_xml_new(DATADIR"/monkey-bubble/glade/netgame.glade","network_window",NULL);
        
        PRIVATE(ngl)->window = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "network_window");
	gtk_widget_hide (glade_xml_get_widget (PRIVATE(ngl)->glade_xml, "close_button"));
#endif
#ifdef MAEMO
	PRIVATE(ngl)->glade_xml = glade_xml_new(DATADIR"/monkey-bubble/glade/netgame.glade","vbox1",NULL);

	PRIVATE(ngl)->window = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(PRIVATE(ngl)->window), _("Network game"));
	container = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "vbox1");
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(PRIVATE(ngl)->window)->vbox), container);
	item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "close_button");
	g_signal_connect_swapped( item,"clicked",GTK_SIGNAL_FUNC(close_signal),ngl);
	gtk_widget_set_size_request (glade_xml_get_widget (PRIVATE(ngl)->glade_xml, "scrolledwindow2"), -1, 298); // FIXME: check if necessary
#endif

       
        item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "go_button");
        g_signal_connect_swapped( item,"clicked",GTK_SIGNAL_FUNC(connect_server_signal),ngl);


        item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "quit_button");
        g_signal_connect_swapped( item,"clicked",GTK_SIGNAL_FUNC(quit_server_signal),ngl);

        item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "network_window");
        g_signal_connect_swapped( item,"delete_event",GTK_SIGNAL_FUNC(quit_signal),ngl);


        item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "ready_button");
        g_signal_connect_swapped( item,"clicked",GTK_SIGNAL_FUNC(ready_signal),ngl);
        
        PRIVATE(ngl)->connection_label = GTK_LABEL(glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "connection_state_label"));
        
       gtk_widget_set_sensitive( glade_xml_get_widget( PRIVATE(ngl)->glade_xml
                                                       , "connected_game_hbox"),
                                 FALSE);


       item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml,"players_treeview");
       list = gtk_list_store_new(3,G_TYPE_STRING,G_TYPE_BOOLEAN,G_TYPE_BOOLEAN);

        column = gtk_tree_view_column_new_with_attributes(_("_Player name"),gtk_cell_renderer_text_new(),
                                                          "text",0, (char *)NULL);

        gtk_tree_view_append_column (GTK_TREE_VIEW (item), column);

        column = gtk_tree_view_column_new();

        column = gtk_tree_view_column_new_with_attributes(_("_Owner"),gtk_cell_renderer_toggle_new(),
                                                          "active",1, (char *)NULL);

        gtk_tree_view_append_column (GTK_TREE_VIEW (item), column);

        column = gtk_tree_view_column_new();

        column = gtk_tree_view_column_new_with_attributes(_("_Ready"),gtk_cell_renderer_toggle_new(),
                                                          "active",2, (char *)NULL);

        gtk_tree_view_append_column (GTK_TREE_VIEW (item), column);


        gtk_tree_view_set_model( GTK_TREE_VIEW(item), GTK_TREE_MODEL(list));

        
        PRIVATE(ngl)->players_list = list;
        
        return ngl;
        
}


struct StatusJob {
        UiNetworkClient * self;
        gchar * message;
} StatusJob;


gboolean status_idle(gpointer data) {
        struct StatusJob * j;
        UiNetworkClient  * self;
        
        j = (struct StatusJob *) data;
        
        self = j->self;
        
        gtk_label_set_label(PRIVATE(self)->connection_label,
                            j->message);
        
        g_free(j->message);
        g_free(j);
        return FALSE;
}



static void set_status_message(UiNetworkClient * self,
                               const gchar * message) {

        struct StatusJob * j;
        j = (struct StatusJob *)g_malloc( sizeof(StatusJob));
        j->self = self;
        j->message = g_strdup(message);
        g_idle_add(status_idle,j);
}

static void connect_server_signal(gpointer    callback_data,
                           guint       callback_action,
                           GtkWidget  *widget) {
        UiNetworkClient  * self;
        GtkWidget * entry;

        self = UI_NETWORK_CLIENT(callback_data);

        entry =  glade_xml_get_widget( PRIVATE(self)->glade_xml, "server_name_entry");
        if( PRIVATE(self)->server_name != NULL) {
                g_free(PRIVATE(self)->server_name);
        }

        PRIVATE(self)->server_name = 
                g_strdup( gtk_entry_get_text(GTK_ENTRY(entry)));

        
        set_status_message(self,  g_strdup_printf("Connecting %s ...",
                                                 PRIVATE(self)->server_name));


        set_sensitive( glade_xml_get_widget( PRIVATE(self)->glade_xml
                                             , "connect_hbox"),FALSE);        
        if( connect_server(self) ) {

        } else {
                set_status_message(self,  g_strdup_printf("Can't connect to %s ...",
                                                              PRIVATE(self)->server_name));


                set_sensitive( glade_xml_get_widget( PRIVATE(self)->glade_xml
                                                     , "connect_hbox"),TRUE);        

        }

}



static void send_disconnect(UiNetworkClient * self) {
        xmlDoc * doc;
        xmlNode * root;
        
        doc = xmlNewDoc((guchar*)"1.0");
        root = xmlNewNode(NULL, (guchar*)"message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,(guchar*)"name",(guchar*)"disconnect");
        
        network_message_handler_send_xml_message(PRIVATE(self)->handler,
                                                PRIVATE(self)->client_id,
                                                doc);

        xmlFreeDoc(doc);

}

static void disconnected(UiNetworkClient * self) {
        g_object_unref( PRIVATE(self)->manager_proxy);
        
        PRIVATE(self)->manager_proxy = NULL;
                
        update_players_list(self);
        set_sensitive( glade_xml_get_widget( PRIVATE(self)->glade_xml
                                             , "connected_game_hbox"),FALSE);
        
        set_sensitive( glade_xml_get_widget( PRIVATE(self)->glade_xml
                                             , "connect_hbox"),TRUE);
                                
}



static gboolean quit_signal(gpointer    callback_data,
                           guint       callback_action,
                               GtkWidget  *widget) {

        UiNetworkClient * self;

        self = UI_NETWORK_CLIENT(callback_data);

        if( PRIVATE(self)->manager_proxy != NULL) {
                g_object_unref( PRIVATE(self)->manager_proxy);
        
                PRIVATE(self)->manager_proxy = NULL;
        }
        

 
        if( PRIVATE(self)->handler != NULL) {
                send_disconnect(self);
                
                network_message_handler_disconnect(PRIVATE(self)->handler);
        
                
                g_object_unref( PRIVATE(self)->handler);
                PRIVATE(self)->handler = NULL;
                
        }



        
        return FALSE;
}


static gboolean quit_server_signal(gpointer    callback_data,
                           guint       callback_action,
                               GtkWidget  *widget) {

        UiNetworkClient * self;

        self = UI_NETWORK_CLIENT(callback_data);

        if( PRIVATE(self)->manager_proxy != NULL) {
                g_object_unref( PRIVATE(self)->manager_proxy);
        
                PRIVATE(self)->manager_proxy = NULL;
        }
        

        if( PRIVATE(self)->handler != NULL) {
                send_disconnect(self);
                
                network_message_handler_disconnect(PRIVATE(self)->handler);
        
                
                g_object_unref( PRIVATE(self)->handler);
                PRIVATE(self)->handler = NULL;
                
        }
        

        update_players_list(self);
        
        set_sensitive( glade_xml_get_widget( PRIVATE(self)->glade_xml
                                             , "connect_hbox"),TRUE); 

        set_sensitive( glade_xml_get_widget( PRIVATE(self)->glade_xml
                                             , "connected_game_hbox"),FALSE); 

        
        return FALSE;
}



static void ready_signal(gpointer    callback_data,
                         guint       callback_action,
                         GtkWidget  *widget) {


        UiNetworkClient * self;
        GtkWidget * item;

        self = UI_NETWORK_CLIENT(callback_data);

        item = glade_xml_get_widget( PRIVATE(self)->glade_xml, "ready_button");

        if( ! PRIVATE(self)->ready ) {
                net_game_manager_proxy_send_ready_state(PRIVATE(self)->manager_proxy,
                                                        TRUE);
                PRIVATE(self)->ready = TRUE;
        } else {

                net_game_manager_proxy_send_ready_state(PRIVATE(self)->manager_proxy,
                                                        FALSE);
                
                PRIVATE(self)->ready = FALSE;
                
        }
        net_game_manager_proxy_send_start( PRIVATE(self)->manager_proxy);

}


void send_init(UiNetworkClient * self) {
        xmlDoc * doc;
        xmlNode * current, * root;
        xmlNode * text;
        
        doc = xmlNewDoc((guchar*)"1.0");
        root = xmlNewNode(NULL, (guchar*)"message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root, (guchar*)"name", (guchar*)"init");
        
        current = xmlNewNode(NULL, (guchar*)"player");
        
        text = xmlNewText((guchar*)g_get_user_name());
        
        xmlAddChild(current,text);
        xmlAddChild(root,current);

        network_message_handler_send_xml_message(PRIVATE(self)->handler,
                                                PRIVATE(self)->client_id,
                                                doc);

        xmlFreeDoc(doc);

}






static gboolean update_players_list_idle(gpointer data) {

        UiNetworkClient * self;

        GList * next;

        self = UI_NETWORK_CLIENT(data);
        gtk_list_store_clear( PRIVATE( self)->players_list);
        
        if( PRIVATE(self)->manager_proxy != NULL) {
                next = net_game_manager_proxy_get_players( PRIVATE(self)->manager_proxy);

                
                while(next != NULL) {
                        GtkTreeIter iter;
                        GtkListStore * tree;
                        Client * client;
                        client = (Client *)next->data;

                        
                        tree =  PRIVATE(self)->players_list;
                        
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

static void update_players_list(UiNetworkClient * self) {
        g_idle_add(update_players_list_idle,(gpointer)self);
        
}

gboolean start_game_idle(gpointer data) {
        UiNetworkClient * self;

        self = UI_NETWORK_CLIENT(data);


        g_print("network-game-laucnehr destroy widget\n");
        gtk_widget_destroy( PRIVATE(self)->window);

        g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(self)->handler ),
                                               G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,self);
              

                      

        g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(self)->manager_proxy ),
                                               G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,self);
                      

                      
        g_print("Network-game-self : game started\n");

        return FALSE;
}

void start_game(UiNetworkClient * self) {
        g_idle_add(start_game_idle,self);
        
}


static void 
players_list_updated(NetGameManagerProxy * proxy,
                     UiNetworkClient * self) 
{

        update_players_list(self);
}

static void 
game_created(NetGameManagerProxy * proxy,
             UiNetworkClient * self) 
{
        start_game(self);
}



static gboolean
update_number_of_players_idle(gpointer data) 
{
        UiNetworkClient * self;
        GtkWidget * item;

        self = UI_NETWORK_CLIENT(data);
        item = glade_xml_get_widget( PRIVATE(self)->glade_xml, "number_of_players");
        gtk_label_set_label(GTK_LABEL(item),g_strdup_printf("%d", net_game_manager_proxy_get_number_of_players(PRIVATE(self)->manager_proxy)));

        return FALSE;
}

static void
update_number_of_players(UiNetworkClient * self)
{
        g_idle_add(update_number_of_players_idle,self);
        
}

static void 
number_of_players_changed(NetGameManagerProxy * proxy,
                              UiNetworkClient * self) {

        update_number_of_players(self);
}



static gboolean
update_number_of_games_idle(gpointer data) 
{
        UiNetworkClient * self;
        GtkWidget * item;

        self = UI_NETWORK_CLIENT(data);
        item = glade_xml_get_widget( PRIVATE(self)->glade_xml, "number_of_games");
        gtk_label_set_label(GTK_LABEL(item),g_strdup_printf("%d", net_game_manager_proxy_get_number_of_games(PRIVATE(self)->manager_proxy)));

        return FALSE;
}

static void
update_number_of_games(UiNetworkClient * self)
{
        g_idle_add(update_number_of_games_idle,self);
        
}

static void 
number_of_games_changed(NetGameManagerProxy * proxy,
                              UiNetworkClient * self) {

        update_number_of_games(self);
}

void recv_network_xml_message(NetworkMessageHandler * mmh,
		  guint32 client_id,
		  xmlDoc * message,
		  gpointer * p) {

        xmlNode * root;
        UiNetworkClient * self;
        guchar * message_name;
        
        root = message->children;
        
        g_assert( g_str_equal(root->name,"message"));
        
        self = UI_NETWORK_CLIENT(p);
        
        message_name = xmlGetProp(root,(guchar*)"name");
        
        if( g_str_equal(message_name,"init_request") ) {
                
                g_print("INIT REQUIEST");
                
                PRIVATE(self)->client_id = client_id;
                
                
                
                send_init(self);
                
                set_status_message(self, g_strdup_printf("Connected to %s",
                                                             PRIVATE(self)->server_name));
                
                
                
                
        } else if(g_str_equal(message_name,"init_reply") ){
                          
                if( g_str_equal(root->children->content,"ok")) {
                        g_print("init ok!!");
                        PRIVATE(self)->manager_proxy =
                                net_game_manager_proxy_new( PRIVATE(self)->handler,client_id);
                        
                        g_signal_connect( PRIVATE(self)->manager_proxy,
                                          "players-list-updated",
                                          G_CALLBACK(players_list_updated),
                                          self);
                        

                        g_signal_connect( PRIVATE(self)->manager_proxy,
                                          "game-created",
                                          G_CALLBACK(game_created),
                                          self);

                        g_signal_connect( PRIVATE(self)->manager_proxy,
                                          "number-of-players-changed",
                                          G_CALLBACK(number_of_players_changed),
                                          self);

                        g_signal_connect( PRIVATE(self)->manager_proxy,
                                          "number-of-games-changed",
                                          G_CALLBACK(number_of_games_changed),
                                          self);

                } else {
                        g_print("init not ok!!");
                        
                        set_sensitive( glade_xml_get_widget( PRIVATE(self)->glade_xml
                                                             , "connect_hbox"),TRUE); 
                        
                        set_status_message(self,"Not ok");
                }
        } else if(g_str_equal(message_name,"game_joined")) {
                int game_id;
                GameNetworkPlayerManager * manager;
                UiMain * ui;
                
                sscanf((gchar*)root->children->content,"%d",&game_id);
                g_print("game id : %d\n",game_id);                
                set_sensitive( glade_xml_get_widget( PRIVATE(self)->glade_xml
                                                     ,"connected_game_hbox"),TRUE); 

                sscanf((gchar*)root->children->content,"%d",&game_id);
             
                
                ui = ui_main_get_instance();
                manager = game_network_player_manager_new(ui_main_get_window(ui),
                                                          ui_main_get_canvas(ui),
                                                          PRIVATE(self)->handler,
                                                          PRIVATE(self)->client_id);
                ui_main_set_game_manager(ui,
                                         GAME_MANAGER(manager));

                g_object_unref( manager);

        } else if( g_str_equal( message_name,"cant_join_game")) {
                set_status_message(self, "Can't join the game");
                set_sensitive( glade_xml_get_widget( PRIVATE(self)->glade_xml
                                                             , "connect_hbox"),TRUE); 
                        
                
        }


        xmlFree(message_name);
        
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

static void connection_closed(NetworkMessageHandler * handler,
                              UiNetworkClient * self) {
        disconnected(self);
}


static gboolean connect_server(UiNetworkClient * self) {
        

        PRIVATE(self)->handler = network_message_handler_new(0);
        
        if( network_message_handler_connect(PRIVATE(self)->handler,PRIVATE(self)->server_name,6666) == FALSE) {
                g_object_unref( G_OBJECT(PRIVATE(self)->handler));
                PRIVATE(self)->handler = NULL;

                return FALSE;
        }
   

        g_signal_connect( G_OBJECT(PRIVATE(self)->handler),
                          "recv-xml-message",
                          G_CALLBACK( recv_network_xml_message ),
                          self);


        g_signal_connect( G_OBJECT(PRIVATE(self)->handler),
                          "connection-closed",
                          G_CALLBACK( connection_closed ),
                          self);

        network_message_handler_start_listening(PRIVATE(self)->handler);

        PRIVATE(self)->ready = FALSE;
        return TRUE;
}


void ui_network_client_finalize(GObject *object) {
        UiNetworkClient * self = (UiNetworkClient *) object;
	
        g_free(PRIVATE(self));

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }		
}

static void ui_network_client_instance_init(UiNetworkClient * self) {
        self->private =g_new0 (UiNetworkClientPrivate, 1);
}

static void ui_network_client_class_init (UiNetworkClientClass *klass) {
        GObjectClass* object_class;
        
        parent_class = g_type_class_peek_parent(klass);
        object_class = G_OBJECT_CLASS(klass);
        object_class->finalize = ui_network_client_finalize;
}

GType ui_network_client_get_type(void) {
        static GType ui_network_client_type = 0;
        
        if (!ui_network_client_type) {
                static const GTypeInfo ui_network_client_info = {
                        sizeof(UiNetworkClientClass),
                        NULL,           /* base_init */
                        NULL,           /* base_finalize */
                        (GClassInitFunc) ui_network_client_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof(UiNetworkClient),
                        1,              /* n_preallocs */
                        (GInstanceInitFunc) ui_network_client_instance_init,
                };
                
                ui_network_client_type = g_type_register_static(G_TYPE_OBJECT,
                                                                     "UiNetworkClient",
                                                                     &ui_network_client_info, 0
                                                                     );

        }
        
        return ui_network_client_type;
}
