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

#ifndef GAME_MANAGER_PROXY_H
#define GAME_MANAGER_PROXY_H

#include <glib-object.h>
#include "message-handler.h"
G_BEGIN_DECLS

typedef struct NetGameManagerProxyPrivate NetGameManagerProxyPrivate;

typedef struct _NetGameManagerProxy NetGameManagerProxy;
typedef struct _NetGameManagerProxyClass NetGameManagerProxyClass;

NetGameManagerProxy *     net_game_manager_proxy_new(NetworkMessageHandler * handler,guint32 client_id);

GList * net_game_manager_proxy_get_players(NetGameManagerProxy * self);
gint net_game_manager_proxy_get_number_of_players(NetGameManagerProxy * self);
gint net_game_manager_proxy_get_number_of_games(NetGameManagerProxy * self);

void net_game_manager_proxy_send_number_of_players(NetGameManagerProxy * self,int n);
void net_game_manager_proxy_send_number_of_games(NetGameManagerProxy * self,int n);

void net_game_manager_proxy_send_ready_state(NetGameManagerProxy * self,gboolean ready);

void net_game_manager_proxy_send_start(NetGameManagerProxy * self);

GType		net_game_manager_proxy_get_type	       (void);

#define NET_TYPE_GAME_MANAGER_PROXY			(net_game_manager_proxy_get_type ())
#define NET_GAME_MANAGER_PROXY(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), NET_TYPE_GAME_MANAGER_PROXY, NetGameManagerProxy))
#define NET_GAME_MANAGER_PROXY_CLASS(klass)		(G_TYPE_CHACK_CLASS_CAST((klass), NET_TYPE_GAME_MANAGER_PROXY, NetGameManagerProxyClass))
#define NET_IS_GAME_MANAGER_PROXY(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), NET_TYPE_GAME_MANAGER_PROXY))
#define NET_IS_GAME_MANAGER_PROXY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), NET_TYPE_GAME_MANAGER_PROXY))
#define NET_GAME_MANAGER_PROXY_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), NET_TYPE_GAME_MANAGER_PROXY, NetGameManagerProxyClass))

struct _NetGameManagerProxy
{
	GObject		   base_instance;
	NetGameManagerProxyPrivate * private;
};

struct _NetGameManagerProxyClass
{
	GObjectClass	  base_class;
	
	void (* players_list_updated) (NetGameManagerProxy * self);
	void (* number_of_players_changed) (NetGameManagerProxy * self);
        void (* number_of_games_changed) (NetGameManagerProxy * self);
	void (* game_created) (NetGameManagerProxy * self);
};

typedef struct Client {
	gchar * name;
	gboolean ready;
	gboolean owner;
	gint id;
} Client;

G_END_DECLS

#endif /* GAME_MANAGER_PROXY_H */
