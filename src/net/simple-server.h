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

#ifndef SIMPLE_SERVER_H
#define SIMPLE_SERVER_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _NetworkSimpleServerPrivate NetworkSimpleServerPrivate;

typedef struct _NetworkSimpleServer NetworkSimpleServer;
typedef struct _NetworkSimpleServerClass NetworkSimpleServerClass;

NetworkSimpleServer *     network_simple_server_new                 ();
gboolean                  network_simple_server_start(NetworkSimpleServer * server);
void                      network_simple_server_stop(NetworkSimpleServer * server);

GType		network_simple_server_get_type	       (void);

#define NETWORK_TYPE_SIMPLE_SERVER			(network_simple_server_get_type ())
#define NETWORK_SIMPLE_SERVER(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), NETWORK_TYPE_SIMPLE_SERVER, NetworkSimpleServer))
#define NETWORK_SIMPLE_SERVER_CLASS(klass)		(G_TYPE_CHACK_CLASS_CAST((klass), NETWORK_TYPE_SIMPLE_SERVER, NetworkSimpleServerClass))
#define NETWORK_IS_SIMPLE_SERVER(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), NETWORK_TYPE_SIMPLE_SERVER))
#define NETWORK_IS_SIMPLE_SERVER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), NETWORK_TYPE_SIMPLE_SERVER))
#define NETWORK_SIMPLE_SERVER_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), NETWORK_TYPE_SIMPLE_SERVER, NetworkSimpleServerClass))

struct _NetworkSimpleServer
{
	GObject		   base_instance;
	NetworkSimpleServerPrivate * private;
};

struct _NetworkSimpleServerClass
{
	GObjectClass	  base_class;
};

G_END_DECLS

#endif /* SIMPLE_SERVER_H */
