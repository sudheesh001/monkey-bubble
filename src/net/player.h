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

#ifndef PLAYER_H
#define PLAYER_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct NetworkPlayerPrivate NetworkPlayerPrivate;

typedef struct _NetworkPlayer NetworkPlayer;
typedef struct _NetworkPlayerClass NetworkPlayerClass;


NetworkPlayer *     network_player_new                 (const gchar * name);
const gchar *   network_player_get_name        (NetworkPlayer * player);
GType		network_player_get_type	       (void);

#define NETWORK_TYPE_PLAYER			(network_player_get_type ())
#define NETWORK_PLAYER(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), NETWORK_TYPE_PLAYER, NetworkPlayer))
#define NETWORK_PLAYER_CLASS(klass)		(G_TYPE_CHACK_CLASS_CAST((klass), NETWORK_TYPE_PLAYER, NetworkPlayerClass))
#define NETWORK_IS_PLAYER(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), NETWORK_TYPE_PLAYER))
#define NETWORK_IS_PLAYER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), NETWORK_TYPE_PLAYER))
#define NETWORK_PLAYER_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), NETWORK_TYPE_PLAYER, NetworkPlayerClass))

struct _NetworkPlayer
{
	GObject		   base_instance;
	NetworkPlayerPrivate * private;
};

struct _NetworkPlayerClass
{
	GObjectClass	  base_class;
};

G_END_DECLS

#endif /* PLAYER_H */
