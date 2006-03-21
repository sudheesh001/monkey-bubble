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
#include <string.h>
#include "game-manager-proxy.h"

#define PRIVATE(self) (self->private )
static GObjectClass* parent_class = NULL;


enum {
	PLAYERS_LIST_UPDATED,
	NUMBER_OF_PLAYERS_CHANGED,
	NUMBER_OF_GAMES_CHANGED,
	GAME_CREATED,
	LAST_SIGNAL

};

static guint32 signals[LAST_SIGNAL];

struct NetGameManagerProxyPrivate 
{
	GList * clients;
	NetworkMessageHandler * handler;
	gint number_of_games;
	gint number_of_players;
	guint32 client_id;
};

static void recv_xml_message(NetworkMessageHandler * handler,
			     guint32 client_id,
			     xmlDoc * message,
			     NetGameManagerProxy * self);

NetGameManagerProxy *
net_game_manager_proxy_new  (NetworkMessageHandler * handler,guint32 client_id)
{
	NetGameManagerProxy * self;

	self = NET_GAME_MANAGER_PROXY( g_object_new(NET_TYPE_GAME_MANAGER_PROXY,NULL) );

	PRIVATE(self)->handler = handler;
	PRIVATE(self)->client_id = client_id;
        g_signal_connect( PRIVATE(self)->handler,
                          "recv-xml-message",
                          G_CALLBACK( recv_xml_message ),
			  self);

	return self;
}

static void 
net_game_manager_proxy_instance_init(NetGameManagerProxy * self) 
{
	PRIVATE(self) = g_new0 (NetGameManagerProxyPrivate, 1);			
	PRIVATE(self)->clients = NULL;
}


static void
net_game_manager_proxy_finalize (GObject * object) 
{
	NetGameManagerProxy * self;

	self = NET_GAME_MANAGER_PROXY(object);


        g_signal_handlers_disconnect_matched(  PRIVATE(self)->handler ,
					       G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,self);
	
	g_free(PRIVATE(self));
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}

}


static void 
free_client(gpointer data,gpointer user_data)
{
	Client * client;

	client = (Client *) data;
	g_free( client->name);
	g_free(client);
}

static void
recv_players_list(NetGameManagerProxy * self,xmlDoc * doc) 
{

	
        xmlNode * root;
        xmlNode * current;

        root = doc->children;

        current = root->children;


        if( PRIVATE(self)->clients != NULL) {
                g_list_foreach(PRIVATE(self)->clients,free_client,NULL);
                
                g_list_free(PRIVATE(self)->clients);
                PRIVATE(self)->clients = NULL;
        }
        
        
        
        while(current != NULL) {
                gboolean owner;
                gboolean ready;
                xmlChar * prop;
                Client * client;
		int id;

	prop = xmlGetProp(current,(guchar*)"owner");
                
                if( prop != NULL && strcmp( (gchar*)prop,(gchar*)"true")  == 0) {
                        owner = TRUE;
                } else {
                        owner = FALSE;
                }
                
                prop = xmlGetProp(current,(guchar*)"ready");                
                if( prop != NULL && strcmp( (gchar*)prop,(gchar*)"true")  == 0) {
                        ready = TRUE;
                } else {
                        ready = FALSE;
                }
                
                client = g_malloc(sizeof(Client));
                client->name = g_strdup((gchar*)current->children->content);
                client->owner = owner; 
                client->ready = ready;

		sscanf((gchar*)xmlGetProp(current,(guchar*)"id"),"%d",&id);
		client->id = id;
                PRIVATE(self)->clients = g_list_append( PRIVATE(self)->clients,
							client);
                current = current->next;
                            
        }

	g_signal_emit( G_OBJECT(self),signals[PLAYERS_LIST_UPDATED],0);

}


static void 
recv_xml_message(NetworkMessageHandler * handler,guint32 client_id,
		 xmlDoc * message,NetGameManagerProxy * self)
{

        xmlNode * root;
        guchar * message_name;
        
        root = message->children;
        
        g_assert( g_str_equal(root->name,"message"));
        
        
        message_name = xmlGetProp(root,(guchar*)"name");
        
        if( g_str_equal(message_name,"game_player_list") ) {
                
                recv_players_list(self,message);
                
        } else if( g_str_equal(message_name,"number_of_games")) {
		int number;
            
		sscanf((gchar*)root->children->content,"%d",&number);
		PRIVATE(self)->number_of_games = number;
		g_signal_emit( G_OBJECT(self),signals[NUMBER_OF_GAMES_CHANGED],0);

	} else if( g_str_equal(message_name,"number_of_players")) {
		int number;
            
		sscanf((gchar*)root->children->content,"%d",&number);
		PRIVATE(self)->number_of_players = number;
		g_signal_emit( G_OBJECT(self),signals[NUMBER_OF_PLAYERS_CHANGED],0);

	}else if(g_str_equal(message_name,"game_created") ) {
                
                int game_id;
                
                sscanf((gchar*)root->children->content,"%d",&game_id);
                g_signal_emit( G_OBJECT(self),signals[GAME_CREATED],0);

        }

}

GList * 
net_game_manager_proxy_get_players(NetGameManagerProxy * self)
{
	return PRIVATE(self)->clients;
}

gint 
net_game_manager_proxy_get_number_of_players(NetGameManagerProxy * self) 
{
	return PRIVATE(self)->number_of_players;
}

gint
net_game_manager_proxy_get_number_of_games(NetGameManagerProxy * self) 
{
	return PRIVATE(self)->number_of_games;
}

void 
net_game_manager_proxy_send_ready_state(NetGameManagerProxy * self,gboolean ready)
{
	xmlDoc * doc;
        xmlNode * root;
        xmlNode * text;
        
        doc = xmlNewDoc((guchar*)"1.0");
        root = xmlNewNode(NULL, (guchar*)"message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,(guchar*)"name",(guchar*)"ready_state");
        
        text = xmlNewText((ready ? (guchar*)"true" : (guchar*)"false"));
        
        xmlAddChild(root,text);

        network_message_handler_send_xml_message(PRIVATE(self)->handler,
						 PRIVATE(self)->client_id,
						 doc);

        xmlFreeDoc(doc);
}


void 
net_game_manager_proxy_send_start(NetGameManagerProxy * self)
{
        xmlDoc * doc;
        xmlNode * root;
        xmlNode * text;
        
        doc = xmlNewDoc((guchar*)"1.0");
        root = xmlNewNode(NULL, (guchar*)"message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,(guchar*)"name", (guchar*)"start_game");
        
        text = xmlNewText((guchar*)"1");
        
        xmlAddChild(root,text);

        network_message_handler_send_xml_message(PRIVATE(self)->handler,
						 PRIVATE(self)->client_id,
						 doc);

        xmlFreeDoc(doc);

}

void
net_game_manager_proxy_send_number_of_players(NetGameManagerProxy * self,int n) 
{

        xmlDoc * doc;
        xmlNode * root;
        xmlNode * text;
        
        doc = xmlNewDoc((guchar*)"1.0");
        root = xmlNewNode(NULL, (guchar*)"message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,(guchar*)"name",(guchar*)"number_of_players");
        
        text = xmlNewText((guchar*)g_strdup_printf("%d",n));
        
        xmlAddChild(root,text);

        network_message_handler_send_xml_message(PRIVATE(self)->handler,
						 PRIVATE(self)->client_id,
						 doc);

        xmlFreeDoc(doc);
}

void
net_game_manager_proxy_send_number_of_games(NetGameManagerProxy * self,int n)
{
        xmlDoc * doc;
        xmlNode * root;
        xmlNode * text;
        
        doc = xmlNewDoc((guchar*)"1.0");
        root = xmlNewNode(NULL, (guchar*)"message");
        xmlDocSetRootElement(doc, root);
        
        xmlNewProp(root,(guchar*)"name", (guchar*)"number_of_games");
        
        text = xmlNewText((guchar*)g_strdup_printf("%d",n));
        
        xmlAddChild(root,text);

        network_message_handler_send_xml_message(PRIVATE(self)->handler,
						 PRIVATE(self)->client_id,
						 doc);

        xmlFreeDoc(doc);
}


static void
net_game_manager_proxy_class_init (NetGameManagerProxyClass	* net_game_manager_proxy_class)
{
	GObjectClass	* g_object_class;

	parent_class = g_type_class_peek_parent(net_game_manager_proxy_class);

	g_object_class = G_OBJECT_CLASS(net_game_manager_proxy_class);
	g_object_class->finalize = net_game_manager_proxy_finalize;


	signals[PLAYERS_LIST_UPDATED] = g_signal_new ("players-list-updated",
						      G_TYPE_FROM_CLASS (net_game_manager_proxy_class),
						      G_SIGNAL_RUN_FIRST |
						      G_SIGNAL_NO_RECURSE,
						      G_STRUCT_OFFSET (NetGameManagerProxyClass,players_list_updated),
						      NULL, NULL,
						      g_cclosure_marshal_VOID__VOID,
						      G_TYPE_NONE,
						      0,NULL);

	
	signals[NUMBER_OF_PLAYERS_CHANGED] = g_signal_new ("number-of-players-changed",
							   G_TYPE_FROM_CLASS (net_game_manager_proxy_class),
							   G_SIGNAL_RUN_FIRST |
							   G_SIGNAL_NO_RECURSE,
							   G_STRUCT_OFFSET (NetGameManagerProxyClass,number_of_players_changed),
							   NULL, NULL,
							   g_cclosure_marshal_VOID__VOID,
							   G_TYPE_NONE,
							   0,NULL);

	
	signals[NUMBER_OF_GAMES_CHANGED] = g_signal_new ("number-of-games-changed",
							 G_TYPE_FROM_CLASS (net_game_manager_proxy_class),
							 G_SIGNAL_RUN_FIRST |
							 G_SIGNAL_NO_RECURSE,
							 G_STRUCT_OFFSET (NetGameManagerProxyClass,number_of_games_changed),
							 NULL, NULL,
							 g_cclosure_marshal_VOID__VOID,
							 G_TYPE_NONE,
							 0,NULL);

	signals[GAME_CREATED] = g_signal_new ("game-created",
					      G_TYPE_FROM_CLASS (net_game_manager_proxy_class),
					      G_SIGNAL_RUN_FIRST |
					      G_SIGNAL_NO_RECURSE,
					      G_STRUCT_OFFSET (NetGameManagerProxyClass,game_created),
					      NULL, NULL,
					      g_cclosure_marshal_VOID__VOID,
					      G_TYPE_NONE,
					      0,NULL);

}


GType
net_game_manager_proxy_get_type (void)
{
	static GType	type = 0;

	if (!type)
	{
		const GTypeInfo info = {
			sizeof (NetGameManagerProxyClass),
			NULL,	/* base initializer */
			NULL,	/* base finalizer */
			(GClassInitFunc)net_game_manager_proxy_class_init,
			NULL,	/* class finalizer */
			NULL,	/* class data */
			sizeof (NetGameManagerProxy),
			1,
			(GInstanceInitFunc) net_game_manager_proxy_instance_init,
			0
		};

		type = g_type_register_static (
				G_TYPE_OBJECT,
				"NetGameManagerProxy",
				&info,
				0);
	}

	return type;
}

