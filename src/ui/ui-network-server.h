/* ui-network-server.h - 
 * Copyright (C) 2004 Laurent Belmonte <lolo3d@tuxfamily.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef _UI_NETWORK_SERVER_H_
#define _UI_NETWORK_SERVER_H_

#include <gtk/gtk.h>
#include "simple-server.h"
#include <net/mb-net-server.h>
G_BEGIN_DECLS

#define TYPE_UI_NETWORK_SERVER   (ui_network_server_get_type())

#define UI_NETWORK_SERVER(object)(G_TYPE_CHECK_INSTANCE_CAST((object),TYPE_UI_NETWORK_SERVER,UiNetworkServer))
#define UI_NETWORK_SERVER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_UI_NETWORK_SERVER,UiNetworkServerClass))
#define IS_UI_NETWORK_SERVER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_UI_NETWORK_SERVER))
#define IS_UI_NETWORK_SERVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_UI_NETWORK_SERVER))
#define UI_NETWORK_SERVER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_UI_NETWORK_SERVER, UiNetworkServerClass))

typedef struct UiNetworkServerPrivate UiNetworkServerPrivate;


typedef struct {
  GObject parent_instance;
  UiNetworkServerPrivate * private;
} UiNetworkServer;


typedef struct {
  GObjectClass parent_class;
} UiNetworkServerClass;


GType ui_network_server_get_type(void);
UiNetworkServer * ui_network_server_new(MbNetServer * server);

G_END_DECLS

#endif
