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
#include <glib/gi18n.h>

#include "message-handler.h"

#include "simple-server.h"
#include "ui-network-server.h"
#include "game-network-player-manager.h"
#include "ui-main.h"
#include "game-manager-proxy.h"

struct UiNetworkServerPrivate
{
  GtkBuilder           * builder;
  GtkWidget            * window;
  gchar                * server_name;
  NetworkMessageHandler* handler;
  int                    client_id;
  gboolean               ready;
  GtkLabel             * connection_label;
  GtkListStore         * players_list;
  NetworkSimpleServer  * manager;
  NetGameManagerProxy  * manager_proxy;
  gboolean               np_from_server;
  gboolean               ng_from_server;
};

#define PRIVATE( UiNetworkServer ) (UiNetworkServer->private)

static GObjectClass* parent_class = NULL;

void ui_network_server_finalize(GObject *);

static void start_game(UiNetworkServer * self);



static void quit_server_signal(gpointer    callback_data,
                           guint       callback_action,
                           GtkWidget  *widget);

static void quit_signal(gpointer    callback_data,
                           guint       callback_action,
                           GtkWidget  *widget);

static void ready_signal(gpointer    callback_data,
                         guint       callback_action,
                         GtkWidget  *widget);


static void number_of_players_changed(UiNetworkServer * self,
                                      GtkWidget  *widget);


static void number_of_games_changed(UiNetworkServer * self,
                                      GtkWidget  *widget);


static void start_signal(gpointer    callback_data,
                         guint       callback_action,
                         GtkWidget  *widget);

static gboolean connect_server(UiNetworkServer * self);


static void set_sensitive(GtkWidget * w,
                          gboolean s);

static void update_players_list(UiNetworkServer * self);

static void recv_network_xml_message(NetworkMessageHandler * mmh,
                              guint32 client_id,
                              xmlDoc * message,
                              gpointer * p);

static void set_number_of_games(UiNetworkServer * self,
                                int n);

static void set_number_of_players(UiNetworkServer * self,
                                  int n);

G_DEFINE_TYPE (UiNetworkServer, ui_network_server, G_TYPE_OBJECT);

UiNetworkServer*
ui_network_server_new (NetworkSimpleServer* server)
{
  GtkTreeViewColumn* column;
  UiNetworkServer  * ngl;
  GtkListStore     * list;
  GtkWidget        * item;
  GError           * error = NULL;
  gchar            * objects[] =
    {
      "network_window",
      NULL
    };

  ngl = UI_NETWORK_SERVER(g_object_new(TYPE_UI_NETWORK_SERVER,
                                       NULL));

  PRIVATE(ngl)->manager = server;
  PRIVATE(ngl)->server_name = NULL;

  PRIVATE(ngl)->ready = FALSE;

  PRIVATE(ngl)->builder = gtk_builder_new ();
  if (0 == gtk_builder_add_objects_from_file (PRIVATE (ngl)->builder, DATADIR "/monkey-bubble/glade/netserver.ui", objects, &error))
    {
      g_warning ("error loading UI for network game server%c %s",
                 error ? ':' : '\0',
                 error ? error->message : "");
      g_error_free (error);
      g_object_unref (ngl);

      return NULL;
    }

  PRIVATE (ngl)->window = GTK_WIDGET (gtk_builder_get_object (PRIVATE (ngl)->builder, "network_window"));

  item = GTK_WIDGET (gtk_builder_get_object ( PRIVATE(ngl)->builder, "quit_button"));
  g_signal_connect_swapped (item, "clicked", G_CALLBACK (quit_server_signal), ngl);

  item = GTK_WIDGET (gtk_builder_get_object ( PRIVATE(ngl)->builder, "network_window"));
  g_signal_connect_swapped (item, "delete-event", G_CALLBACK (quit_signal), ngl);

  item = GTK_WIDGET (gtk_builder_get_object ( PRIVATE(ngl)->builder, "ready_button"));
  g_signal_connect_swapped (item, "clicked", G_CALLBACK (ready_signal), ngl);

  item = GTK_WIDGET (gtk_builder_get_object ( PRIVATE(ngl)->builder, "start_button"));
  g_signal_connect_swapped (item, "clicked", G_CALLBACK (start_signal), ngl);

  item = GTK_WIDGET (gtk_builder_get_object ( PRIVATE(ngl)->builder,"players_treeview"));
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

  item = GTK_WIDGET (gtk_builder_get_object ( PRIVATE(ngl)->builder,"number_of_players"));
  g_signal_connect_swapped (item, "value_changed",
                            G_CALLBACK (number_of_players_changed), ngl);

  item = GTK_WIDGET (gtk_builder_get_object ( PRIVATE(ngl)->builder,"number_of_games"));
  g_signal_connect_swapped (item, "value_changed",
                            G_CALLBACK (number_of_games_changed), ngl);

  PRIVATE(ngl)->players_list = list;

  PRIVATE(ngl)->server_name = "localhost";

  connect_server(ngl);

  return ngl;
}


struct StatusJob {
        UiNetworkServer * self;
        gchar * message;
} StatusJob;



static void 
number_of_players_changed(UiNetworkServer * self,
                          GtkWidget  *widget)
{

        if( PRIVATE(self)->np_from_server == FALSE ) {
                net_game_manager_proxy_send_number_of_players(PRIVATE(self)->manager_proxy,
                                                              gtk_spin_button_get_value_as_int ( GTK_SPIN_BUTTON(widget)));
        }

        
}



static void 
number_of_games_changed(UiNetworkServer * self,
                          GtkWidget  *widget)
{

        if( PRIVATE(self)->ng_from_server == FALSE) {
                net_game_manager_proxy_send_number_of_games(PRIVATE(self)->manager_proxy,
                                                            gtk_spin_button_get_value_as_int ( GTK_SPIN_BUTTON(widget)));
        }

        
}





static void send_disconnect(UiNetworkServer * self) {
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

static void quit_server_signal(gpointer    callback_data,
                           guint       callback_action,
                               GtkWidget  *widget) {


        UiNetworkServer * self;

        self = UI_NETWORK_SERVER(callback_data);

        g_print("destroy \n");

        quit_signal(callback_data,callback_action,widget);
        gtk_widget_destroy( PRIVATE(self)->window);

}

static void quit_signal(gpointer    callback_data,
                           guint       callback_action,
                        GtkWidget  *widget) {

        UiNetworkServer * self;

        self = UI_NETWORK_SERVER(callback_data);


        update_players_list(self);

        if( PRIVATE(self)->handler != NULL) {
                send_disconnect(self);
                
                network_message_handler_disconnect(PRIVATE(self)->handler);
        
                g_object_unref( PRIVATE(self)->handler);
                PRIVATE(self)->handler = NULL;

        }


        network_simple_server_stop(PRIVATE(self)->manager);
        
        g_object_unref( PRIVATE(self)->manager);
        

}

static void
ready_signal (gpointer   callback_data,
              guint      callback_action,
              GtkWidget* widget)
{
  UiNetworkServer * self = UI_NETWORK_SERVER(callback_data);
  gboolean          ready = PRIVATE (self)->ready;


  net_game_manager_proxy_send_ready_state(PRIVATE(self)->manager_proxy,
                                          !ready);
  PRIVATE (self)->ready = !ready;
}

static void start_signal(gpointer   callback_data,
                         guint      callback_action,
                         GtkWidget* widget)
{
  UiNetworkServer * self = UI_NETWORK_SERVER(callback_data);

  net_game_manager_proxy_send_start( PRIVATE(self)->manager_proxy);

  set_sensitive (GTK_WIDGET (gtk_builder_get_object (PRIVATE (self)->builder, "start_button")), FALSE);
  set_sensitive (GTK_WIDGET (gtk_builder_get_object (PRIVATE (self)->builder, "quit_button")), FALSE);
  set_sensitive (GTK_WIDGET (gtk_builder_get_object (PRIVATE (self)->builder, "ready_button")), FALSE);
}


static void send_init(UiNetworkServer * self) {
        xmlDoc * doc;
        xmlNode * current, * root;
        xmlNode * text;
        
        doc = xmlNewDoc((guchar*)"1.0");
        root = xmlNewNode(NULL, (guchar*)"message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,(guchar*)"name",(guchar*)"init");
        
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

        UiNetworkServer * self;

        GList * next;

        self = UI_NETWORK_SERVER(data);
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

static void update_players_list(UiNetworkServer * self) {
        g_idle_add(update_players_list_idle,(gpointer)self);
        
}



static gboolean start_game_idle(gpointer data) {
        UiNetworkServer * self;


        self = UI_NETWORK_SERVER(data);

        g_print("network-server-laucnehr destroy widget\n");
        gtk_widget_destroy( PRIVATE(self)->window);

        g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(self)->handler ),
                                               G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,self);
                      

        g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(self)->manager_proxy ),
                                               G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,self);
                      
        g_print("Network-server-self : game started\n");


        return FALSE;
}

static void 
players_list_updated(NetGameManagerProxy * proxy,
                     UiNetworkServer * self) 
{

        update_players_list(self);
}






static gboolean
update_number_of_players_idle (gpointer data)
{
  UiNetworkServer* self = UI_NETWORK_SERVER(data);
  GtkWidget      * item;

  item = GTK_WIDGET (gtk_builder_get_object (PRIVATE (self)->builder, "number_of_players"));

  /* FIXME: this looks like a replacement for g_signal_handler_block() */
  PRIVATE (self)->np_from_server = TRUE;
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (item),
                             net_game_manager_proxy_get_number_of_players (PRIVATE (self)->manager_proxy));

  PRIVATE (self)->np_from_server = FALSE;
  return FALSE;
}

static void
update_number_of_players(UiNetworkServer * self)
{
        g_idle_add(update_number_of_players_idle,self);
        
}

static void 
net_number_of_players_changed(NetGameManagerProxy * proxy,
                          UiNetworkServer * self) 
{
        update_number_of_players(self);
}


static gboolean
update_number_of_games_idle (gpointer data)
{
  UiNetworkServer* self = UI_NETWORK_SERVER(data);
  GtkWidget      * item;

  item = GTK_WIDGET (gtk_builder_get_object (PRIVATE (self)->builder, "number_of_games"));
  /* FIXME: this looks like a replacement for g_signal_handler_block() */
  PRIVATE (self)->ng_from_server = TRUE;
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (item),
                             net_game_manager_proxy_get_number_of_games (PRIVATE (self)->manager_proxy));
  PRIVATE (self)->ng_from_server = FALSE;
  return FALSE;
}

static void
update_number_of_games(UiNetworkServer * self)
{
        g_idle_add(update_number_of_games_idle,self);
        
}

static void 
net_number_of_games_changed(NetGameManagerProxy * proxy,
                          UiNetworkServer * self) 
{
        update_number_of_games(self);
}


static void 
game_created(NetGameManagerProxy * proxy,
             UiNetworkServer * self) 
{
        start_game(self);
}

static void start_game(UiNetworkServer * self) {
        g_idle_add(start_game_idle,self);
        
}

static void recv_network_xml_message(NetworkMessageHandler * mmh,
		  guint32 client_id,
		  xmlDoc * message,
		  gpointer * p) {

        xmlNode * root;
        UiNetworkServer * self;
        guchar * message_name;
        
        root = message->children;
        
        g_assert( g_str_equal(root->name,"message"));
        
        self = UI_NETWORK_SERVER(p);
        
        message_name = xmlGetProp(root,(guchar*)"name");
        
        if( g_str_equal(message_name,"game_player_list") ) {
                
                
        } else if( g_str_equal(message_name,"init_request") ) {
                
                
                PRIVATE(self)->client_id = client_id;
                
                
                
                send_init(self);
                
                
                
                
        } else if(g_str_equal(message_name,"init_reply") ){
                          
                if( g_str_equal(root->children->content,"ok")) {
                        g_print("network-server-self : init ok\n");
                        PRIVATE(self)->manager_proxy =
                                net_game_manager_proxy_new( PRIVATE(self)->handler,client_id);

                        g_signal_connect( PRIVATE(self)->manager_proxy,
                                          "players-list-updated",
                                          G_CALLBACK(players_list_updated),
                                          self);

                        g_signal_connect( PRIVATE(self)->manager_proxy,
                                          "number-of-players-changed",
                                          G_CALLBACK(net_number_of_players_changed),
                                          self);

                        g_signal_connect( PRIVATE(self)->manager_proxy,
                                          "number-of-games-changed",
                                          G_CALLBACK(net_number_of_games_changed),
                                          self);


                        g_signal_connect( PRIVATE(self)->manager_proxy,
                                          "game-created",
                                          G_CALLBACK(game_created),
                                          self);

                } else {
                        g_print("network-server-launcer : init not ok\n");
                        
                }
        } else if(g_str_equal(message_name,"game_joined")) {
                int game_id;
                GameNetworkPlayerManager * manager;
                UiMain * ui;

                sscanf((gchar*)root->children->content,"%d",&game_id);
             
                
                ui = ui_main_get_instance();
                manager = game_network_player_manager_new(ui_main_get_window(ui),
                                                          ui_main_get_canvas(ui),
                                                          PRIVATE(self)->handler,
                                                          PRIVATE(self)->client_id);
                ui_main_set_game_manager(ui,
                                         GAME_MANAGER(manager));

                g_object_unref(manager);
        } else if( g_str_equal(message_name,"number_of_games")) {
		int number;
            
		sscanf((gchar*)root->children->content,"%d",&number);
                set_number_of_games(self,number);
	} else if( g_str_equal(message_name,"number_of_players")) {
		int number;
            
		sscanf((gchar*)root->children->content,"%d",&number);
                set_number_of_players(self,number);
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
set_number_of_games(UiNetworkServer * self,
                    int n) 
{
}

static void 
set_number_of_players(UiNetworkServer * self,
                      int n) 
{
        g_print("number of players %d\n",n);
}


static gboolean connect_server(UiNetworkServer * self) {

        PRIVATE(self)->handler = network_message_handler_new(0);
        if( ! network_message_handler_connect( PRIVATE(self)->handler,
                                              PRIVATE(self)->server_name,
                                              6666)  ) {
        
                g_object_unref( G_OBJECT(PRIVATE(self)->handler));
                PRIVATE(self)->handler = NULL;
                return FALSE;
        }
   

        g_signal_connect( G_OBJECT(PRIVATE(self)->handler),
                          "recv-xml-message",
                          G_CALLBACK( recv_network_xml_message ),
                          self);


        network_message_handler_start_listening(PRIVATE(self)->handler);

        PRIVATE(self)->ready = FALSE;
        return TRUE;
}


void ui_network_server_finalize(GObject *object) {
        UiNetworkServer * self = (UiNetworkServer *) object;
	
        g_free(PRIVATE(self));

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }
}

static void
ui_network_server_init (UiNetworkServer* self)
{
	self->private = g_new0 (UiNetworkServerPrivate, 1);
        PRIVATE(self)->np_from_server = FALSE;
        PRIVATE(self)->ng_from_server = FALSE;
}

static void ui_network_server_class_init (UiNetworkServerClass *klass) {
        GObjectClass* object_class;
        
        parent_class = g_type_class_peek_parent(klass);
        object_class = G_OBJECT_CLASS(klass);
        object_class->finalize = ui_network_server_finalize;
}

/* vim:set et sw=2 cino=t0,f0,(0,{s,>2s,n-1s,^-1s,e2s: */
