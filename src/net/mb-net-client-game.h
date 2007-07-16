/* this file is part of monkey-bubble
 *
 * AUTHORS
 *       Laurent Belminte        <laurent.belmonte@gmail.com>
 *
 * Copyright (C) 2007 Laurent Belmonte
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
#include <net/mb-net-handler-manager.h>
#include <net/mb-net-client-match.h>
#ifndef _MB_NET__CLIENT_GAME_H
#define _MB_NET__CLIENT_GAME_H

G_BEGIN_DECLS typedef struct _MbNetClientGame MbNetClientGame;
typedef struct _MbNetClientGameClass MbNetClientGameClass;

GType mb_net_client_game_get_type(void);
void mb_net_client_game__init(MbNetClientGame * self, guint32 id,
			      MbNetConnection * con,
			      MbNetHandlerManager * manager);

void mb_net_client_game_join(MbNetClientGame * self);
void mb_net_client_game_start(MbNetClientGame * self);
void mb_net_client_game_stop(MbNetClientGame * self);
void mb_net_client_game_ask_player_list(MbNetClientGame * self);
void mb_net_client_game_ask_score(MbNetClientGame * self);
GList *mb_net_client_game_get_players(MbNetClientGame * self);
GList *mb_net_client_game_get_score(MbNetClientGame * self);
gboolean mb_net_client_game_is_master(MbNetClientGame * self);

#define MB_NET_TYPE_CLIENT_GAME			(mb_net_client_game_get_type())
#define MB_NET_CLIENT_GAME(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_NET_TYPE_CLIENT_GAME, MbNetClientGame))
#define MB_NET_CLIENT_GAME_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MB_NET_TYPE_CLIENT_GAME, MbNetClientGameClass))
#define MB_NET_IS_CLIENT_GAME(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_NET_TYPE_CLIENT_GAME))
#define MB_NET_IS_CLIENT_GAME_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_NET_TYPE_CLIENT_GAME))
#define MB_NET_CLIENT_GAME_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_NET_TYPE_CLIENT_GAME, MbNetClientGameClass))

struct _MbNetClientGame {
	GObject base_instance;
};

struct _MbNetClientGameClass {
	GObjectClass base_class;

	/* signals */

	void (*join_response) (MbNetClientGame * self, gboolean ok);
	void (*player_list_changed) (MbNetClientGame * self);
	void (*score_changed) (MbNetClientGame * self);
	void (*start) (MbNetClientGame * self, MbNetClientMatch * match);
	void (*stop) (MbNetClientGame * self);
	void (*current_match) (MbNetClientMatch * match);
};

G_END_DECLS
#endif				/* !_MB_NET__CLIENT_GAME_H */
