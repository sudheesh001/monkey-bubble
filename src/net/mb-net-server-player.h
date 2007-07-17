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

#ifndef _MB_NET__SERVER_PLAYER_H
#define _MB_NET__SERVER_PLAYER_H

#include <net/mb-net-connection.h>
G_BEGIN_DECLS typedef struct _MbNetServerPlayer MbNetServerPlayer;
typedef struct _MbNetServerPlayerClass MbNetServerPlayerClass;

GType mb_net_server_player_get_type(void);

MbNetServerPlayer *mb_net_server_player_new(MbNetConnection * con,
					    guint32 player_id,
					    const gchar * name);
const gchar *mb_net_server_player_get_name(MbNetServerPlayer * self);
guint32 mb_net_server_player_get_id(MbNetServerPlayer * self);
#define MB_NET_TYPE_SERVER_PLAYER			(mb_net_server_player_get_type())
#define MB_NET_SERVER_PLAYER(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_NET_TYPE_SERVER_PLAYER, MbNetServerPlayer))
#define MB_NET_SERVER_PLAYER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MB_NET_TYPE_SERVER_PLAYER, MbNetServerPlayerClass))
#define MB_NET_IS_SERVER_PLAYER(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_NET_TYPE_SERVER_PLAYER))
#define MB_NET_IS_SERVER_PLAYER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_NET_TYPE_SERVER_PLAYER))
#define MB_NET_SERVER_PLAYER_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_NET_TYPE_SERVER_PLAYER, MbNetServerPlayerClass))

struct _MbNetServerPlayer {
	GObject base_instance;
};

struct _MbNetServerPlayerClass {
	GObjectClass base_class;

	/* signals */
	void (*disconnected) (MbNetServerPlayer * self);
};

G_END_DECLS
#endif				/* !_MB_NET::_SERVER_PLAYER_H */
