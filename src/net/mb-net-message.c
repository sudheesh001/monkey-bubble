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

#include <string.h>
#include <arpa/inet.h>
#include <glib.h>
#include <glib-object.h>

#include "mb-net-message.h"




typedef struct _Private {
	GArray *message;
	guint current_read;
} Private;




enum {
	PROP_ATTRIBUTE
};


static GObjectClass *parent_class = NULL;

static void mb_net_message_get_property(GObject * object,
					guint prop_id,
					GValue * value,
					GParamSpec * param_spec);
static void mb_net_message_set_property(GObject * object,
					guint prop_id,
					const GValue * value,
					GParamSpec * param_spec);



G_DEFINE_TYPE_WITH_CODE(MbNetMessage, mb_net_message, G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_MESSAGE, Private))




static void mb_net_message_finalize(MbNetMessage * self);

static void mb_net_message_init(MbNetMessage * self);



static void mb_net_message_init(MbNetMessage * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->current_read = 0;
	priv->message = g_array_new(FALSE, FALSE, sizeof(guint8));
}


static void mb_net_message_finalize(MbNetMessage * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_array_free(priv->message, TRUE);
	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}

MbNetMessage *mb_net_message_create()
{
	MbNetMessage *self =
	    MB_NET_MESSAGE(g_object_new(MB_NET_TYPE_MESSAGE, NULL));
	return self;
}


MbNetMessage *mb_net_message_create_from(const guint8 * data, guint32 size)
{
	Private *priv;

	MbNetMessage *self =
	    MB_NET_MESSAGE(g_object_new(MB_NET_TYPE_MESSAGE, NULL));
	priv = GET_PRIVATE(self);
	g_array_append_vals(priv->message, data, size);
	return self;
}

MbNetMessage *mb_net_message_new(guint32 handler_id,
				 guint32 dst_handler_id, guint32 action_id)
{
	Private *priv;

	MbNetMessage *self =
	    MB_NET_MESSAGE(g_object_new(MB_NET_TYPE_MESSAGE, NULL));
	priv = GET_PRIVATE(self);
	mb_net_message_add_int(self, handler_id);
	mb_net_message_add_int(self, dst_handler_id);
	mb_net_message_add_int(self, action_id);
	return self;

}

void mb_net_message_read_init(MbNetMessage * self, guint32 * handler_id,
			      guint32 * dst_handler_id,
			      guint32 * action_id)
{
	(*handler_id) = mb_net_message_read_int(self);
	(*dst_handler_id) = mb_net_message_read_int(self);
	(*action_id) = mb_net_message_read_int(self);
}

guint32 mb_net_message_size(MbNetMessage * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	return priv->message->len;
}

const guint8 *mb_net_message_data(MbNetMessage * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	return (const guint8 *) priv->message->data;
}

void mb_net_message_add_int(MbNetMessage * self, guint32 value)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	guint32 v = htonl(value);
	g_array_append_vals(priv->message, &v, sizeof(value));

}

void mb_net_message_add_boolean(MbNetMessage * self, gboolean value)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	guint8 v = (value == TRUE ? 1 : 0);
	g_array_append_val(priv->message, v);
}

void mb_net_message_add_string(MbNetMessage * self, const gchar * str)
{
	if (str == NULL)
		return;
	Private *priv;
	priv = GET_PRIVATE(self);
	guint32 size = strlen(str);
	mb_net_message_add_int(self, size);
	g_array_append_vals(priv->message, (guint8 *) str, size);
}

void mb_net_message_skip(MbNetMessage * self, guint32 skip_size)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->current_read += skip_size;
}

guint32 mb_net_message_read_int(MbNetMessage * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	guint32 ret = 0;
	if (priv->current_read > priv->message->len) {
		return 0;
	}

	if ((priv->current_read + sizeof(guint32)) > priv->message->len) {
		return 0;
	}

	guint8 *p = (guint8 *) priv->message->data;
	p += priv->current_read;
	ret = *((guint32 *) p);
	priv->current_read += sizeof(guint32);
	return g_ntohl(ret);
}

gboolean mb_net_message_read_boolean(MbNetMessage * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	guint8 ret = 0;
	if (priv->current_read > priv->message->len) {
		return FALSE;
	}

	if ((priv->current_read + sizeof(guint8)) > priv->message->len) {
		return FALSE;
	}

	guint8 *p = (guint8 *) priv->message->data;
	p += priv->current_read;
	ret = *p;
	priv->current_read += sizeof(guint8);
	return (ret == 1 ? TRUE : FALSE);
}


gchar *mb_net_message_read_string(MbNetMessage * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	guint32 size = mb_net_message_read_int(self);
	if (size == 0) {
		return NULL;
	}


	if ((priv->current_read + size) > priv->message->len) {
		return NULL;
	}

	guint8 *p = (guint8 *) priv->message->data;
	p += priv->current_read;
	gchar *r = (gchar *) g_new0(guint8, size + 1);
	memcpy(r, p, size);

	priv->current_read += size;
	return r;
}

/*

MbNetMessage * mb_net_message_create();
MbNetMessage * mb_net_message_create_from(const guint8 * data,guint32 size);
void mb_net_message_add_int(MbNetMessage * self,guint32 value);
void mb_net_message_add_string(MbNetMessage * self,const gchar * string);
void mb_net_message_add_xmldoc(MbNetMessage * self,const xmlDoc * doc);

void mb_net_message_skip(MbNetMessage * self,guint32 skip_size);
guint32 mb_net_message_read_int(MbNetMessage * self);
gboolean mb_net_message_read_boolean(MbNetMessage * self);
gchar * mb_net_message_read_string(MbNetMessage * self);
xmlDoc * mb_net_message_read_xmldoc(MbNetMessage * self);


*/

static void
mb_net_message_get_property(GObject * object, guint prop_id,
			    GValue * value, GParamSpec * param_spec)
{
	MbNetMessage *self;

	self = MB_NET_MESSAGE(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_message_set_property(GObject * object, guint prop_id,
			    const GValue * value, GParamSpec * param_spec)
{
	MbNetMessage *self;

	self = MB_NET_MESSAGE(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}


static void
mb_net_message_class_init(MbNetMessageClass * mb_net_message_class)
{
	GObjectClass *g_object_class;

	parent_class = g_type_class_peek_parent(mb_net_message_class);


	g_type_class_add_private(mb_net_message_class, sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_message_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_message_set_property;
	g_object_class->get_property = mb_net_message_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_message_finalize;


}
