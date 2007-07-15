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
#include <net/mb-net-handler.h>

#ifndef _MB_NET__HANDLER_MANAGER_H
#define _MB_NET__HANDLER_MANAGER_H

G_BEGIN_DECLS typedef struct _MbNetHandlerManager MbNetHandlerManager;
typedef struct _MbNetHandlerManagerClass MbNetHandlerManagerClass;

GType mb_net_handler_manager_get_type(void);

void mb_net_handler_manager_register(MbNetHandlerManager * self,
				     MbNetHandler * h);
void mb_net_handler_manager_unregister(MbNetHandlerManager * self,
				       guint32 handler_id);
void mb_net_handler_manager_message(MbNetHandlerManager * self,
				    MbNetConnection * con,
				    guint32 handler_id,
				    guint32 tohandler_id, guint32 action,
				    MbNetMessage * m);
#define MB_NET_TYPE_HANDLER_MANAGER			(mb_net_handler_manager_get_type())
#define MB_NET_HANDLER_MANAGER(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_NET_TYPE_HANDLER_MANAGER, MbNetHandlerManager))
#define MB_NET_HANDLER_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MB_NET_TYPE_HANDLER_MANAGER, MbNetHandlerManagerClass))
#define MB_NET_IS_HANDLER_MANAGER(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_NET_TYPE_HANDLER_MANAGER))
#define MB_NET_IS_HANDLER_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_NET_TYPE_HANDLER_MANAGER))
#define MB_NET_HANDLER_MANAGER_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_NET_TYPE_HANDLER_MANAGER, MbNetHandlerManagerClass))

struct _MbNetHandlerManager {
	GObject base_instance;
};

struct _MbNetHandlerManagerClass {
	GObjectClass base_class;

	/* signals */
};

G_END_DECLS
#endif				/* !_MB_NET__HANDLER_MANAGER_H */
