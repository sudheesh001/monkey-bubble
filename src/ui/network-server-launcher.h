/* network-server-launcher.h - 
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
#ifndef _NETWORK_SERVER_LAUNCHER_H_
#define _NETWORK_SERVER_LAUNCHER_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_NETWORK_SERVER_LAUNCHER   (network_server_launcher_get_type())

#define NETWORK_SERVER_LAUNCHER(object)(G_TYPE_CHECK_INSTANCE_CAST((object),TYPE_NETWORK_SERVER_LAUNCHER,NetworkServerLauncher))
#define NETWORK_SERVER_LAUNCHER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_NETWORK_SERVER_LAUNCHER,NetworkServerLauncherClass))
#define IS_NETWORK_SERVER_LAUNCHER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_NETWORK_SERVER_LAUNCHER))
#define IS_NETWORK_SERVER_LAUNCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_NETWORK_SERVER_LAUNCHER))
#define NETWORK_SERVER_LAUNCHER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_NETWORK_SERVER_LAUNCHER, NetworkServerLauncherClass))

typedef struct NetworkServerLauncherPrivate NetworkServerLauncherPrivate;


typedef struct {
  GObject parent_instance;
  NetworkServerLauncherPrivate * private;
} NetworkServerLauncher;


typedef struct {
  GObjectClass parent_class;
} NetworkServerLauncherClass;


GType network_server_launcher_get_type(void);
NetworkServerLauncher * network_server_launcher_new(void);

G_END_DECLS

#endif
