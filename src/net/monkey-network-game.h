/* monkey-network-game.h - 
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
#ifndef _MONKEY_NETWORK_GAME_H_
#define _MONKEY_NETWORK_GAME_H_

#include <glib-object.h>
#include "monkey-message-handler.h"

G_BEGIN_DECLS

#define TYPE_MONKEY_NETWORK_GAME   (monkey_network_game_get_type())

#define MONKEY_NETWORK_GAME(object)(G_TYPE_CHECK_INSTANCE_CAST((object),TYPE_MONKEY_NETWORK_GAME,MonkeyNetworkGame))
#define MONKEY_NETWORK_GAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_MONKEY_NETWORK_GAME,MonkeyNetworkGameClass))
#define IS_MONKEY_NETWORK_GAME(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_MONKEY_NETWORK_GAME))
#define IS_MONKEY_NETWORK_GAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_MONKEY_NETWORK_GAME))
#define MONKEY_NETWORK_GAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_MONKEY_NETWORK_GAME, MonkeyNetworkGameClass))

typedef struct MonkeyNetworkGamePrivate MonkeyNetworkGamePrivate;


typedef struct {
  GObject parent_instance;
  MonkeyNetworkGamePrivate * private;
} MonkeyNetworkGame;


typedef struct {
  GObjectClass parent_class;

	 /* signals */
	 void ( * game_stopped) (MonkeyNetworkGame * g);
} MonkeyNetworkGameClass;

typedef struct NetworkGame NetworkGame;


typedef struct NetworkClient {
        guint          client_id;
        gchar *        client_name;
        gboolean       ready;
  MonkeyMessageHandler * handler;
  NetworkGame * game;
} NetworkClient;



struct NetworkGame {
        guint                 game_id;
        MonkeyNetworkGame *   game;
        GList *               clients;
        NetworkClient *       game_owner;
};


GType monkey_network_game_get_type(void);
MonkeyNetworkGame * monkey_network_game_new(NetworkGame * ng);

G_END_DECLS

#endif
