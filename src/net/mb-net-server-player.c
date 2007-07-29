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

#include "mb-net-server-player.h"

#include <glib.h>
#include <glib-object.h>


typedef struct _Private {

	MbNetConnection *con;
	gchar *name;
	guint32 player_id;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	DISCONNECTED,
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_server_player_get_property(GObject * object,
					      guint prop_id,
					      GValue * value,
					      GParamSpec * param_spec);
static void mb_net_server_player_set_property(GObject * object,
					      guint prop_id,
					      const GValue * value,
					      GParamSpec * param_spec);


static guint _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetServerPlayer, mb_net_server_player,
			G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_SERVER_PLAYER, Private))




static void mb_net_server_player_finalize(MbNetServerPlayer * self);

static void mb_net_server_player_init(MbNetServerPlayer * self);


void _disconnected(MbNetConnection * con, MbNetServerPlayer * self)
{
	g_print("disconnected \n");
	g_signal_connect_swapped(con, "disconnected",
				 (GCallback) _disconnected, self);

	g_signal_emit(self, _signals[DISCONNECTED], 0);
}

MbNetServerPlayer *mb_net_server_player_new(MbNetConnection * con,
					    guint32 player_id,
					    const gchar * name)
{
	MbNetServerPlayer *self;
	self =
	    MB_NET_SERVER_PLAYER(g_object_new
				 (MB_NET_TYPE_SERVER_PLAYER, NULL));
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->name = g_strdup(name);
	priv->con = con;
	g_object_ref(con);
	priv->player_id = player_id;

	g_signal_connect(con, "disconnected", (GCallback) _disconnected,
			 self);
	return self;
}

static void mb_net_server_player_init(MbNetServerPlayer * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
}


const gchar *mb_net_server_player_get_name(MbNetServerPlayer * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	return priv->name;
}

guint32 mb_net_server_player_get_id(MbNetServerPlayer * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	return priv->player_id;
}

static void mb_net_server_player_finalize(MbNetServerPlayer * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	g_object_unref(priv->con);
	g_free(priv->name);
	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}

static void
mb_net_server_player_get_property(GObject * object, guint prop_id,
				  GValue * value, GParamSpec * param_spec)
{
	MbNetServerPlayer *self;

	self = MB_NET_SERVER_PLAYER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_server_player_set_property(GObject * object, guint prop_id,
				  const GValue * value,
				  GParamSpec * param_spec)
{
	MbNetServerPlayer *self;

	self = MB_NET_SERVER_PLAYER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_server_player_class_init(MbNetServerPlayerClass *
				mb_net_server_player_class)
{
	GObjectClass *g_object_class;

	parent_class =
	    g_type_class_peek_parent(mb_net_server_player_class);


	g_type_class_add_private(mb_net_server_player_class,
				 sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_server_player_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_server_player_set_property;
	g_object_class->get_property = mb_net_server_player_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_server_player_finalize;

	_signals[DISCONNECTED] =
	    g_signal_new("disconnected", MB_NET_TYPE_SERVER_PLAYER,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetServerPlayerClass,
					 disconnected), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0,
			 NULL);


}
