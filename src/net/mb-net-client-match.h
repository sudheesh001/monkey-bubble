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

#ifndef _MB_NET__CLIENT_MATCH_H
#define _MB_NET__CLIENT_MATCH_H

#include <monkey/monkey.h>
#include <net/mb-net-match-handler.h>
#include <net/mb-net-handler-manager.h>
#include <monkey/monkey.h>
G_BEGIN_DECLS typedef struct _MbNetClientMatch MbNetClientMatch;
typedef struct _MbNetClientMatchClass MbNetClientMatchClass;

GType mb_net_client_match_get_type(void);

MbNetClientMatch *mb_net_client_match_new(guint32 match_id,
					  guint32 player_id,
					  MbNetConnection * con,
					  MbNetHandlerManager * manager);
void mb_net_client_match_ready(MbNetClientMatch * self);
void mb_net_client_match_shoot(MbNetClientMatch * self);
void mb_net_client_match_lock(MbNetClientMatch * self);
void mb_net_client_match_unlock(MbNetClientMatch * self);
Monkey *mb_net_client_match_get_monkey(MbNetClientMatch * self);

#define MB_NET_TYPE_CLIENT_MATCH			(mb_net_client_match_get_type())
#define MB_NET_CLIENT_MATCH(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_NET_TYPE_CLIENT_MATCH, MbNetClientMatch))
#define MB_NET_CLIENT_MATCH_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MB_NET_TYPE_CLIENT_MATCH, MbNetClientMatchClass))
#define MB_NET_IS_CLIENT_MATCH(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_NET_TYPE_CLIENT_MATCH))
#define MB_NET_IS_CLIENT_MATCH_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_NET_TYPE_CLIENT_MATCH))
#define MB_NET_CLIENT_MATCH_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_NET_TYPE_CLIENT_MATCH, MbNetClientMatchClass))

struct _MbNetClientMatch {
	GObject base_instance;
};

struct _MbNetClientMatchClass {
	GObjectClass base_class;

	/* signals */
	void (*winlost) (MbNetClientMatch * self, gboolean win);
	void (*start) (MbNetClientMatch * self);
	void (*stop) (MbNetClientMatch * self);
};

G_END_DECLS
#endif				/* !_MB_NET::_CLIENT_MATCH_H */
