/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* this file is part of criawips, a gnome presentation application
 *
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

#ifndef GAME_H
#define GAME_H

#include <glib-object.h>
#include "client.h"

G_BEGIN_DECLS

typedef struct NetworkGamePrivate NetworkGamePrivate;

typedef struct _NetworkGame NetworkGame;
typedef struct _NetworkGameClass NetworkGameClass;

NetworkGame *     network_game_new                 (GList * clients);

void network_game_start(NetworkGame * game);
NetworkClient * network_game_get_winner(NetworkGame * game);

GType		network_game_get_type	       (void);

#define NETWORK_TYPE_GAME			(network_game_get_type ())
#define NETWORK_GAME(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), NETWORK_TYPE_GAME, NetworkGame))
#define NETWORK_GAME_CLASS(klass)		(G_TYPE_CHACK_CLASS_CAST((klass), NETWORK_TYPE_GAME, NetworkGameClass))
#define NETWORK_IS_GAME(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), NETWORK_TYPE_GAME))
#define NETWORK_IS_GAME_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), NETWORK_TYPE_GAME))
#define NETWORK_GAME_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), NETWORK_TYPE_GAME, NetworkGameClass))

struct _NetworkGame
{
	GObject		   base_instance;
	NetworkGamePrivate * private;
};

struct _NetworkGameClass
{
	GObjectClass	  base_class;
	// signals
	
	void (* game_stopped )(NetworkGame * game);
};

G_END_DECLS

#endif /* GAME_H */
