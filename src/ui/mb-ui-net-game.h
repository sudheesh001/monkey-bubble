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

#ifndef _MB_UI__NET_GAME_H
#define _MB_UI__NET_GAME_H
#include "game.h"

#include <net/mb-net-client-game.h>
#include <net/mb-net-client-match.h>
G_BEGIN_DECLS typedef struct _MbUiNetGame MbUiNetGame;
typedef struct _MbUiNetGameClass MbUiNetGameClass;

GType mb_ui_net_game_get_type(void);
MbUiNetGame *mb_ui_net_game_new(MbNetClientGame * game,
				MbNetClientMatch * match);
#define MB_UI_TYPE_NET_GAME			(mb_ui_net_game_get_type())
#define MB_UI_NET_GAME(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_UI_TYPE_NET_GAME, MbUiNetGame))
#define MB_UI_NET_GAME_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MB_UI_TYPE_NET_GAME, MbUiNetGameClass))
#define MB_UI_IS_NET_GAME(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_UI_TYPE_NET_GAME))
#define MB_UI_IS_NET_GAME_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_UI_TYPE_NET_GAME))
#define MB_UI_NET_GAME_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_UI_TYPE_NET_GAME, MbUiNetGameClass))

struct _MbUiNetGame {
	Game base_instance;
};

struct _MbUiNetGameClass {
	GameClass base_class;

	/* signals */
};

G_END_DECLS
#endif				/* !_MB_UI::_NET_GAME_H */
