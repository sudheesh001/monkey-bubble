/* this file is part of monkey-bubble
 *
 * AUTHORS
 *       Laurent Belminte        <laurent.belmonte@gmail.com>
 *
 * Copyright (C) 2007 Laurent Belmonte
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

#ifndef _MB_NET__CLIENT_SERVER_H
#define _MB_NET__CLIENT_SERVER_H
#include <net/mb-net-client-game.h>
G_BEGIN_DECLS typedef struct _MbNetClientServer MbNetClientServer;
typedef struct _MbNetClientServerClass MbNetClientServerClass;

GType mb_net_client_server_get_type(void);
void mb_net_client_server_set_name(MbNetClientServer * self,
				   const gchar * name);
void mb_net_client_server_disconnect(MbNetClientServer * self);

void mb_net_client_server_connect(MbNetClientServer * self,
				  const gchar * uri, GError ** error);
void mb_net_client_server_ask_games(MbNetClientServer * self,
				    GError ** error);
GList *mb_net_client_server_get_games(MbNetClientServer * self);
void mb_net_client_server_create_game(MbNetClientServer * self,
				      const gchar * name, GError ** error);

MbNetClientGame *mb_net_client_server_create_client(MbNetClientServer *
						    self, guint32 game_id);

#define MB_NET_TYPE_CLIENT_SERVER			(mb_net_client_server_get_type())
#define MB_NET_CLIENT_SERVER(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_NET_TYPE_CLIENT_SERVER, MbNetClientServer))
#define MB_NET_CLIENT_SERVER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MB_NET_TYPE_CLIENT_SERVER, MbNetClientServerClass))
#define MB_NET_IS_CLIENT_SERVER(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_NET_TYPE_CLIENT_SERVER))
#define MB_NET_IS_CLIENT_SERVER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_NET_TYPE_CLIENT_SERVER))
#define MB_NET_CLIENT_SERVER_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_NET_TYPE_CLIENT_SERVER, MbNetClientServerClass))

struct _MbNetClientServer {
	GObject base_instance;
};

struct _MbNetClientServerClass {
	GObjectClass base_class;

	/* signals */
	void (*connected) (MbNetClientServer * self, gboolean ok);
	void (*new_game_list) (MbNetClientServer * self);
	void (*game_created) (MbNetClientServer * self,
			      MbNetClientGame * game);
};

G_END_DECLS
#endif				/* !_MB_NET::_CLIENT_SERVER_H */
