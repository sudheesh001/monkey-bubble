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

#include "mb-net-match.h"

#include <glib.h>
#include <glib-object.h>


typedef struct _Private {

} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_match_get_property(GObject * object,
				      guint prop_id,
				      GValue * value,
				      GParamSpec * param_spec);
static void mb_net_match_set_property(GObject * object,
				      guint prop_id,
				      const GValue * value,
				      GParamSpec * param_spec);


//static        guint   _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetMatch, mb_net_match, G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_MATCH, Private))




static void mb_net_match_finalize(MbNetMatch * self);

static void mb_net_match_init(MbNetMatch * self);



static void mb_net_match_init(MbNetMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
}


static void mb_net_match_finalize(MbNetMatch * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}

static void
mb_net_match_get_property(GObject * object, guint prop_id, GValue * value,
			  GParamSpec * param_spec)
{
	MbNetMatch *self;

	self = MB_NET_MATCH(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_match_set_property(GObject * object, guint prop_id,
			  const GValue * value, GParamSpec * param_spec)
{
	MbNetMatch *self;

	self = MB_NET_MATCH(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void mb_net_match_class_init(MbNetMatchClass * mb_net_match_class)
{
	GObjectClass *g_object_class;

	parent_class = g_type_class_peek_parent(mb_net_match_class);


	g_type_class_add_private(mb_net_match_class, sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_match_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_match_set_property;
	g_object_class->get_property = mb_net_match_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_match_finalize;


}
