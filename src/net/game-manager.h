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

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <glib-object.h>
#include "client.h"
G_BEGIN_DECLS

typedef struct NetworkGameManagerPrivate NetworkGameManagerPrivate;

typedef struct _NetworkGameManager NetworkGameManager;
typedef struct _NetworkGameManagerClass NetworkGameManagerClass;

NetworkGameManager *     network_game_manager_new                 (void);
gboolean network_game_manager_add_client(NetworkGameManager * manager,
					 NetworkClient * client);

GType		network_game_manager_get_type	       (void);

#define NETWORK_TYPE_GAME_MANAGER			(network_game_manager_get_type ())
#define NETWORK_GAME_MANAGER(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), NETWORK_TYPE_GAME_MANAGER, NetworkGameManager))
#define NETWORK_GAME_MANAGER_CLASS(klass)		(G_TYPE_CHACK_CLASS_CAST((klass), NETWORK_TYPE_GAME_MANAGER, NetworkGameManagerClass))
#define NETWORK_IS_GAME_MANAGER(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), NETWORK_TYPE_GAME_MANAGER))
#define NETWORK_IS_GAME_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), NETWORK_TYPE_GAME_MANAGER))
#define NETWORK_GAME_MANAGER_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), NETWORK_TYPE_GAME_MANAGER, NetworkGameManagerClass))

struct _NetworkGameManager
{
	GObject		   base_instance;
	NetworkGameManagerPrivate * private;
};

struct _NetworkGameManagerClass
{
	GObjectClass	  base_class;
};

G_END_DECLS

#endif /* GAME_MANAGER_H */
