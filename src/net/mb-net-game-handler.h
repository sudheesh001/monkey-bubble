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
#include <net/mb-net-game.h>
#include <net/mb-net-abstract-handler.h>
#ifndef _MB_NET__GAME_HANDLER_H
#define _MB_NET__GAME_HANDLER_H

G_BEGIN_DECLS typedef struct _MbNetGameHandler MbNetGameHandler;
typedef struct _MbNetGameHandlerClass MbNetGameHandlerClass;

GType mb_net_game_handler_get_type(void);

#define MB_NET_TYPE_GAME_HANDLER			(mb_net_game_handler_get_type())
#define MB_NET_GAME_HANDLER(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_NET_TYPE_GAME_HANDLER, MbNetGameHandler))
#define MB_NET_GAME_HANDLER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MB_NET_TYPE_GAME_HANDLER, MbNetGameHandlerClass))
#define MB_NET_IS_GAME_HANDLER(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_NET_TYPE_GAME_HANDLER))
#define MB_NET_IS_GAME_HANDLER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_NET_TYPE_GAME_HANDLER))
#define MB_NET_GAME_HANDLER_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_NET_TYPE_GAME_HANDLER, MbNetGameHandlerClass))

struct _MbNetGameHandler {
	GObject base_instance;

};

struct _MbNetGameHandlerClass {
	GObjectClass base_class;

	/* signals */
};

G_END_DECLS
#endif				/* !_MB_NET::_GAME_HANDLER_H */
