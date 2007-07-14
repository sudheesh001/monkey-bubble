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

#include "mb-net-abstract-handler.h"

#include <glib.h>
#include <glib-object.h>


typedef struct _Private {
	int handler_id;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_abstract_handler_iface_init(MbNetHandlerInterface *
					       iface);

static void mb_net_abstract_handler_get_property(GObject * object,
						 guint prop_id,
						 GValue * value,
						 GParamSpec * param_spec);
static void mb_net_abstract_handler_set_property(GObject * object,
						 guint prop_id,
						 const GValue * value,
						 GParamSpec * param_spec);


//static        guint   _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetAbstractHandler, mb_net_abstract_handler,
			G_TYPE_OBJECT, {
			G_IMPLEMENT_INTERFACE(MB_NET_TYPE_HANDLER,
					      mb_net_abstract_handler_iface_init)});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_ABSTRACT_HANDLER, Private))


static guint32 _get_id(MbNetHandler * handler);
static void _set_id(MbNetHandler * handler, guint32 handler_id);
static void _receive(MbNetHandler * handler, MbNetConnection * con,
		     guint32 src_handler_id, guint32 dst_handler_id,
		     guint32 action_id, MbNetMessage * m);
static void mb_net_abstract_handler_finalize(MbNetAbstractHandler * self);

static void mb_net_abstract_handler_init(MbNetAbstractHandler * self);



static void mb_net_abstract_handler_init(MbNetAbstractHandler * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->handler_id = 0;
}


static void mb_net_abstract_handler_finalize(MbNetAbstractHandler * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}


static guint32 _get_id(MbNetHandler * handler)
{
	return GET_PRIVATE(MB_NET_ABSTRACT_HANDLER(handler))->handler_id;
}

static void _set_id(MbNetHandler * handler, guint32 handler_id)
{
	GET_PRIVATE(MB_NET_ABSTRACT_HANDLER(handler))->handler_id =
	    handler_id;
}


static void
_receive(MbNetHandler * handler, MbNetConnection * con,
	 guint32 src_handler_id, guint32 dst_handler_id, guint32 action_id,
	 MbNetMessage * m)
{
}

static void
mb_net_abstract_handler_get_property(GObject * object, guint prop_id,
				     GValue * value,
				     GParamSpec * param_spec)
{
	MbNetAbstractHandler *self;

	self = MB_NET_ABSTRACT_HANDLER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_abstract_handler_set_property(GObject * object, guint prop_id,
				     const GValue * value,
				     GParamSpec * param_spec)
{
	MbNetAbstractHandler *self;

	self = MB_NET_ABSTRACT_HANDLER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_abstract_handler_iface_init(MbNetHandlerInterface * iface)
{
	iface->receive = _receive;
	iface->get_id = _get_id;
	iface->set_id = _set_id;
}

static void
mb_net_abstract_handler_class_init(MbNetAbstractHandlerClass *
				   mb_net_abstract_handler_class)
{
	GObjectClass *g_object_class;

	parent_class =
	    g_type_class_peek_parent(mb_net_abstract_handler_class);


	g_type_class_add_private(mb_net_abstract_handler_class,
				 sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_abstract_handler_class);

	/* setting up property system */
	g_object_class->set_property =
	    mb_net_abstract_handler_set_property;
	g_object_class->get_property =
	    mb_net_abstract_handler_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_abstract_handler_finalize;


}

/*

xmlDoc *mb_net_abstract_handler_read_xml_message(const gpointer data,
						 guint32 size,
						 guint32 offset)
{
	if (offset >= size) {
		return NULL;
	} else {
		xmlDocPtr doc;
		xmlParserCtxt *ctxt;

		ctxt = xmlNewParserCtxt();
		if (ctxt == NULL) {
			fprintf(stderr,
				"Failed to allocate parser context\n");
			return NULL;
		}

		doc =
		    xmlCtxtReadDoc(ctxt, (xmlChar *) (data + offset), NULL,
				   NULL, 0);
		return doc;

	}
}*/
