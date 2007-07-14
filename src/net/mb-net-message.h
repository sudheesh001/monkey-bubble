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
#include <libxml/tree.h>

#ifndef _MB_NET__MESSAGE_H
#define _MB_NET__MESSAGE_H

G_BEGIN_DECLS typedef struct _MbNetMessage MbNetMessage;
typedef struct _MbNetMessageClass MbNetMessageClass;

GType mb_net_message_get_type(void);

MbNetMessage *mb_net_message_new(guint32 handler_id,
				 guint32 dst_handler_id,
				 guint32 action_id);
MbNetMessage *mb_net_message_create();
MbNetMessage *mb_net_message_create_from(const guint8 * data,
					 guint32 size);
void mb_net_message_read_init(MbNetMessage * self, guint32 * handler_id,
			      guint32 * dst_handler_id,
			      guint32 * action_id);

guint32 mb_net_message_size(MbNetMessage * self);
const guint8 *mb_net_message_data(MbNetMessage * self);
void mb_net_message_add_int(MbNetMessage * self, guint32 value);
void mb_net_message_add_boolean(MbNetMessage * self, gboolean value);
void mb_net_message_add_string(MbNetMessage * self, const gchar * string);
void mb_net_message_add_xmldoc(MbNetMessage * self, const xmlDoc * doc);

void mb_net_message_skip(MbNetMessage * self, guint32 skip_size);
guint32 mb_net_message_read_int(MbNetMessage * self);
gboolean mb_net_message_read_boolean(MbNetMessage * self);
gchar *mb_net_message_read_string(MbNetMessage * self);
xmlDoc *mb_net_message_read_xmldoc(MbNetMessage * self);

#define MB_NET_TYPE_MESSAGE			(mb_net_message_get_type())
#define MB_NET_MESSAGE(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_NET_TYPE_MESSAGE, MbNetMessage))
#define MB_NET_MESSAGE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MB_NET_TYPE_MESSAGE, MbNetMessageClass))
#define MB_NET_IS_MESSAGE(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_NET_TYPE_MESSAGE))
#define MB_NET_IS_MESSAGE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_NET_TYPE_MESSAGE))
#define MB_NET_MESSAGE_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_NET_TYPE_MESSAGE, MbNetMessageClass))

struct _MbNetMessage {
	GObject base_instance;
};

struct _MbNetMessageClass {
	GObjectClass base_class;

	/* signals */
};

G_END_DECLS
#endif				/* !_MB_NET::_MESSAGE_H */
