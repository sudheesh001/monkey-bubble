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

#ifndef CLIENT_H
#define CLIENT_H

#include <glib-object.h>

#include "player.h"
#include "monkey-message-handler.h"

G_BEGIN_DECLS

#define NETWORK_CLIENT_READY TRUE
#define NETWORK_CLIENT_NOT_READY FALSE
typedef struct NetworkClientPrivate NetworkClientPrivate;

typedef struct _NetworkClient NetworkClient;
typedef struct _NetworkClientClass NetworkClientClass;

NetworkClient * network_client_new(guint32 client_id);


void network_client_set_player(NetworkClient * client,
			       NetworkPlayer * player);

NetworkPlayer * network_client_get_player(NetworkClient * client);


void network_client_set_handler(NetworkClient * client,
				MonkeyMessageHandler * handler);

MonkeyMessageHandler * network_client_get_handler(NetworkClient * client);

gboolean network_client_get_state(NetworkClient * client);

guint32 network_client_get_id(NetworkClient * client);

GType		network_client_get_type	       (void);

#define NETWORK_TYPE_CLIENT			(network_client_get_type ())
#define NETWORK_CLIENT(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), NETWORK_TYPE_CLIENT, NetworkClient))
#define NETWORK_CLIENT_CLASS(klass)		(G_TYPE_CHACK_CLASS_CAST((klass), NETWORK_TYPE_CLIENT, NetworkClientClass))
#define NETWORK_IS_CLIENT(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), NETWORK_TYPE_CLIENT))
#define NETWORK_IS_CLIENT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), NETWORK_TYPE_CLIENT))
#define NETWORK_CLIENT_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), NETWORK_TYPE_CLIENT, NetworkClientClass))

struct _NetworkClient
{
	GObject		   base_instance;
	NetworkClientPrivate * private;

	guint32 id;
};

struct _NetworkClientClass
{
	GObjectClass	  base_class;

	// signals

	void (* state_changed ) (NetworkClient * self);
	void (* disconnect_request) (NetworkClient * self);
	void (* start_request) (NetworkClient * self);
	void (* disconnected) (NetworkClient * self);
};

G_END_DECLS

#endif /* CLIENT_H */
