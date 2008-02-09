/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * AUTHORS
 *       Laurent Belmonte        <laurent.belmonte@aliacom.fr>
 *
 * Copyright (C) 2004 Laurent Belmonte
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
#include <glib.h>

#include "player.h"

#define PRIVATE(self) (self->private )
static GObjectClass* parent_class = NULL;

struct NetworkPlayerPrivate
{
	gchar * name;
};

G_DEFINE_TYPE (NetworkPlayer, network_player, G_TYPE_OBJECT);

NetworkPlayer *
network_player_new  (const gchar * name)
{
	NetworkPlayer * self;

	self = NETWORK_PLAYER( g_object_new(NETWORK_TYPE_PLAYER,NULL) );
	PRIVATE(self)->name = g_strdup(name);
	return self;
}

const gchar *
network_player_get_name (NetworkPlayer * player) 
{
	return PRIVATE(player)->name;
}

static void
network_player_init (NetworkPlayer* self)
{
	PRIVATE(self) = g_new0 (NetworkPlayerPrivate, 1);
	PRIVATE(self)->name = NULL;
}


static void
network_player_finalize (GObject * object) 
{
	NetworkPlayer * self;

	self = NETWORK_PLAYER(object);

	g_free(PRIVATE(self));
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}

}


static void
network_player_class_init (NetworkPlayerClass	* network_player_class)
{
	GObjectClass	* g_object_class;

	parent_class = g_type_class_peek_parent(network_player_class);

	g_object_class = G_OBJECT_CLASS(network_player_class);
	g_object_class->finalize = network_player_finalize;

}

