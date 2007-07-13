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
#ifndef _MB_NET__HANDLER_H
#define _MB_NET__HANDLER_H

G_BEGIN_DECLS typedef struct _MbNetHandler MbNetHandler;
typedef struct _MbNetHandlerInterface MbNetHandlerInterface;

GType mb_net_handler_get_type(void);

void mb_net_handler_receive(MbNetHandler * self, MbNetConnection * con,
			    MbNetMessage * m);

#define MB_NET_TYPE_HANDLER			(mb_net_handler_get_type())
#define MB_NET_HANDLER(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_NET_TYPE_HANDLER, MbNetHandler))
#define MB_NET_IS_HANDLER(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_NET_TYPE_HANDLER))
#define MB_NET_HANDLER_GET_INTERFACE(object)	(G_TYPE_INSTANCE_GET_INTERFACE((object), MB_NET_TYPE_HANDLER, MbNetHandlerInterface))


struct _MbNetHandlerInterface {
	GTypeInterface parent;

	void (*receive) (MbNetHandler * self, MbNetConnection * con,
			 MbNetMessage * m);
	/* signals */
};

G_END_DECLS
#endif				/* !_MB_NET__HANDLER_H */
