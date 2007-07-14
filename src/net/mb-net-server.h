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
#include <net/mb-net-connection.h>
#ifndef _MB_NET__SERVER_H
#define _MB_NET__SERVER_H

G_BEGIN_DECLS typedef struct _MbNetServerPlayer MbNetServerPlayer;
typedef struct _MbNetServer MbNetServer;
typedef struct _MbNetServerClass MbNetServerClass;

GType mb_net_server_get_type(void);

void mb_net_server_accept_on(MbNetServer * self, const gchar * uri,
			     GError ** error);
void mb_net_server_stop(MbNetServer * self);
#define MB_NET_TYPE_SERVER			(mb_net_server_get_type())
#define MB_NET_SERVER(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_NET_TYPE_SERVER, MbNetServer))
#define MB_NET_SERVER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MB_NET_TYPE_SERVER, MbNetServerClass))
#define MB_NET_IS_SERVER(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_NET_TYPE_SERVER))
#define MB_NET_IS_SERVER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_NET_TYPE_SERVER))
#define MB_NET_SERVER_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_NET_TYPE_SERVER, MbNetServerClass))

struct _MbNetServerPlayer {
	MbNetConnection *con;
	guint32 handler_id;
	gchar *name;
};

struct _MbNetServer {
	GObject base_instance;
};

struct _MbNetServerClass {
	GObjectClass base_class;

	/* signals */
	void (*new_player) (MbNetServer * self, MbNetServerPlayer * p);
};

G_END_DECLS
#endif				/* !_MB_NET__SERVER_H */
