/* game_network_player.h
 * Copyright (C) 2002 Laurent Belmonte
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

#ifndef GAME_NETWORK_PLAYER_H
#define GAME_NETWORK_PLAYER_H

#include <gtk/gtk.h>
#include "game.h"
#include "monkey-canvas.h"
#include "monkey.h"
G_BEGIN_DECLS

#define TYPE_GAME_NETWORK_PLAYER            (game_network_player_get_type())

#define GAME_NETWORK_PLAYER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_GAME_NETWORK_PLAYER,GameNetworkPlayer))
#define GAME_NETWORK_PLAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GAME_NETWORK_PLAYER,GameNetworkPlayerClass))
#define IS_GAME_NETWORK_PLAYER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_GAME_NETWORK_PLAYER))
#define IS_GAME_NETWORK_PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_GAME_NETWORK_PLAYER))
#define GAME_NETWORK_PLAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_GAME_NETWORK_PLAYER, GameNetworkPlayerClass))

typedef struct GameNetworkPlayerPrivate GameNetworkPlayerPrivate;



typedef struct {
  Game parent_instance;
  GameNetworkPlayerPrivate * private;
} GameNetworkPlayer;

typedef struct {
  GameClass parent_class;
} GameNetworkPlayerClass;


GType game_network_player_get_type(void);

GameNetworkPlayer * game_network_player_new(GtkWidget * window,MonkeyCanvas * canvas,int level,gint score);

gint game_network_player_get_score(GameNetworkPlayer * g);
gboolean game_network_player_is_lost(GameNetworkPlayer * g);

G_END_DECLS





#endif
