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

#include <net/mb-net-message.h>
#ifndef _MB_NET__HOLDERS_H
#define _MB_NET__HOLDERS_H

G_BEGIN_DECLS typedef struct _MbNetGameListHolder MbNetGameListHolder;
typedef struct _MbNetSimpleGameHolder MbNetSimpleGameHolder;
typedef struct _MbNetPlayerHolder MbNetPlayerHolder;
typedef struct _MbNetPlayerListHolder MbNetPlayerListHolder;
typedef struct _MbNetScoreHolder MbNetScoreHolder;
typedef struct _MbNetPlayerScoreHolder MbNetPlayerScoreHolder;

MbNetGameListHolder *mb_net_game_list_holder_parse(MbNetMessage * m);
void mb_net_game_list_holder_serialize(MbNetGameListHolder * holder,
				       MbNetMessage * m);
void mb_net_game_list_holder_free(MbNetGameListHolder * holder);


MbNetPlayerListHolder *mb_net_player_list_holder_parse(MbNetMessage * m);
void mb_net_player_list_holder_serialize(MbNetPlayerListHolder * holder,
					 MbNetMessage * m);
void mb_net_player_list_holder_free(MbNetPlayerListHolder * holder);


MbNetScoreHolder *mb_net_score_holder_parse(MbNetMessage * m);
void mb_net_score_holder_serialize(MbNetScoreHolder * holder,
				   MbNetMessage * m);
void mb_net_score_holder_free(MbNetScoreHolder * holder);

MbNetPlayerHolder *mb_net_player_holder_parse(MbNetMessage * m);
void mb_net_player_holder_serialize(MbNetPlayerHolder * holder,
				    MbNetMessage * m);
void mb_net_player_holder_free(MbNetPlayerHolder * holder);
MbNetPlayerHolder *mb_net_player_holder_create(const gchar * name);
MbNetSimpleGameHolder *mb_net_simple_game_holder_create(guint handler_id,
							const gchar *
							name);

void mb_net_simple_game_holder_free(MbNetSimpleGameHolder * holder);

struct _MbNetGameListHolder {
	GList *games;
};

struct _MbNetPlayerListHolder {
	GList *players;
};

struct _MbNetScoreHolder {
	GList *score_by_player;
};

struct _MbNetPlayerScoreHolder {
	gchar *name;
	guint score;
};

struct _MbNetSimpleGameHolder {
	guint handler_id;
	gchar *name;
};

struct _MbNetPlayerHolder {
	gchar *name;
};

G_END_DECLS
#endif				/* !_MB_NET__SERVER_HANDLER_H */
