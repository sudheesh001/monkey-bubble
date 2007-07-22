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

#include "mb-net-handler-manager.h"

#include <glib.h>
#include <glib-object.h>


typedef struct _Private {
	GHashTable *map;
	guint32 current_id;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_handler_manager_get_property(GObject * object,
						guint prop_id,
						GValue * value,
						GParamSpec * param_spec);
static void mb_net_handler_manager_set_property(GObject * object,
						guint prop_id,
						const GValue * value,
						GParamSpec * param_spec);


//static        guint   _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetHandlerManager, mb_net_handler_manager,
			G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_HANDLER_MANAGER, Private))




static void mb_net_handler_manager_finalize(MbNetHandlerManager * self);

static void mb_net_handler_manager_init(MbNetHandlerManager * self);



static void mb_net_handler_manager_init(MbNetHandlerManager * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->map = g_hash_table_new(g_direct_hash, g_direct_equal);
}

static void _unref_handler(gpointer key, gpointer data)
{
	g_object_unref(data);
}

static void mb_net_handler_manager_finalize(MbNetHandlerManager * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	g_hash_table_foreach(priv->map, (GHFunc) _unref_handler, NULL);
	g_hash_table_unref(priv->map);
	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}


void mb_net_handler_manager_register(MbNetHandlerManager * self,
				     MbNetHandler * h)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	guint32 id = priv->current_id;
	priv->current_id++;
	mb_net_handler_set_id(h, id);
	g_object_ref(h);
	g_hash_table_insert(priv->map, (gpointer) id, h);


}

void mb_net_handler_manager_unregister(MbNetHandlerManager * self,
				       guint32 handler_id)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	MbNetHandler *h = (MbNetHandler *) g_hash_table_lookup(priv->map,
							       (gpointer)
							       handler_id);
	if (h != NULL) {
		g_hash_table_remove(priv->map, (gpointer) handler_id);
		g_object_unref(h);
	}
}

void mb_net_handler_manager_message(MbNetHandlerManager * self,
				    MbNetConnection * con,
				    guint32 handler_id,
				    guint32 tohandler_id, guint32 action,
				    MbNetMessage * m)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	MbNetHandler *h = (MbNetHandler *) g_hash_table_lookup(priv->map,
							       (gpointer)
							       tohandler_id);

	if (h != NULL) {
		mb_net_handler_receive(h, con, handler_id, tohandler_id,
				       action, m);

	} else {
		g_print
		    ("handler not found ... to : %d action %d con %d \n",
		     tohandler_id, action, (guint32) con);
	}
}



static void
mb_net_handler_manager_get_property(GObject * object, guint prop_id,
				    GValue * value,
				    GParamSpec * param_spec)
{
	MbNetHandlerManager *self;

	self = MB_NET_HANDLER_MANAGER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_handler_manager_set_property(GObject * object, guint prop_id,
				    const GValue * value,
				    GParamSpec * param_spec)
{
	MbNetHandlerManager *self;

	self = MB_NET_HANDLER_MANAGER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_handler_manager_class_init(MbNetHandlerManagerClass *
				  mb_net_handler_manager_class)
{
	GObjectClass *g_object_class;

	parent_class =
	    g_type_class_peek_parent(mb_net_handler_manager_class);


	g_type_class_add_private(mb_net_handler_manager_class,
				 sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_handler_manager_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_handler_manager_set_property;
	g_object_class->get_property = mb_net_handler_manager_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_handler_manager_finalize;


}
