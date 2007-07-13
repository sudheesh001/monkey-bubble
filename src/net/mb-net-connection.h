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
#include <net/mb-net-message.h>
#ifndef _MB_NET__CONNECTION_H
#define _MB_NET__CONNECTION_H

G_BEGIN_DECLS typedef struct _MbNetConnection MbNetConnection;
typedef struct _MbNetConnectionClass MbNetConnectionClass;

enum
{
  MB_NET_CONNECTION_BAD_URI,
  MB_NET_CONNECTION_SOCKET_ERROR
};

GType mb_net_connection_get_type (void);
void mb_net_connection_connect (MbNetConnection * self,
				const gchar * uri, GError ** error);
void mb_net_connection_accept_on (MbNetConnection * self, const gchar * uri,
				  GError ** error);

void mb_net_connection_stop (MbNetConnection * self, GError ** error);
void mb_net_connection_listen (MbNetConnection * self, GError ** error);
void mb_net_connection_disconnect (MbNetConnection * self);

/*
void mb_net_connection_send_message(MbNetConnection * self, guint32 size,
				    const guint8 * data, GError ** error);*/
void mb_net_connection_send_message (MbNetConnection * self,
				     MbNetMessage * message, GError ** error);

#define MB_NET_TYPE_CONNECTION			(mb_net_connection_get_type())
#define MB_NET_CONNECTION(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_NET_TYPE_CONNECTION, MbNetConnection))
#define MB_NET_CONNECTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MB_NET_TYPE_CONNECTION, MbNetConnectionClass))
#define MB_NET_IS_CONNECTION(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_NET_TYPE_CONNECTION))
#define MB_NET_IS_CONNECTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_NET_TYPE_CONNECTION))
#define MB_NET_CONNECTION_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_NET_TYPE_CONNECTION, MbNetConnectionClass))

struct _MbNetConnection
{
  GObject base_instance;
};

struct _MbNetConnectionClass
{
  GObjectClass base_class;

  /* signals */
  void (*receive) (MbNetConnection * self, guint32 size, const guint8 * data);

  void (*receive_message) (MbNetConnection * self, MbNetMessage * m);
  void (*new_connection) (MbNetConnection * self,
			  MbNetConnection * new_connection);
  void (*disconnected) (MbNetConnection * self);
};

G_END_DECLS
#endif /* !_MB_NET__CONNECTION_H */
