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

#include "mb-net-handler.h"

#include <glib.h>
#include <glib-object.h>

void mb_net_handler_receive(MbNetHandler * self,
			    MbNetConnection * con,
			    guint32 source_handler_id,
			    guint32 dest_handler_id,
			    guint32 action_id, MbNetMessage * m)
{
	MB_NET_HANDLER_GET_INTERFACE(self)->receive(self, con,
						    source_handler_id,
						    dest_handler_id,
						    action_id, m);
}

guint32 mb_net_handler_get_id(MbNetHandler * self)
{
	return MB_NET_HANDLER_GET_INTERFACE(self)->get_id(self);
}

void mb_net_handler_set_id(MbNetHandler * self, guint32 id)
{
	MB_NET_HANDLER_GET_INTERFACE(self)->set_id(self, id);
}

void mb_net_handler_receive(MbNetHandler * self,
			    MbNetConnection * con,
			    guint32 source_handler_id,
			    guint32 dest_handler_id,
			    guint32 action_id, MbNetMessage * m);

static void mb_net_handler_init(gpointer g_class)
{
	static gboolean initialized = FALSE;
	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

GType mb_net_handler_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof(MbNetHandlerInterface),
			mb_net_handler_init,	/* base_init */
			NULL,	/* base_finalize */
			NULL,	/* class_init */
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			0,
			0,	/* n_preallocs */
			NULL	/* instance_init */
		};
		type =
		    g_type_register_static(G_TYPE_INTERFACE,
					   "MbNetHandler", &info, 0);
	}
	return type;
}
