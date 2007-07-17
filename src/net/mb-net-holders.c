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

#include "mb-net-holders.h"

#include <glib.h>
#include <glib-object.h>

MbNetPlayerListHolder *mb_net_player_list_holder_parse(MbNetMessage * m)
{
	MbNetPlayerListHolder *holder;
	holder = g_new0(MbNetPlayerListHolder, 1);
	guint32 size = mb_net_message_read_int(m);
	int i;
	for (i = 0; i < size; i++) {
		MbNetPlayerHolder *h = mb_net_player_holder_parse(m);
		holder->players = g_list_append(holder->players, h);
	}
	return holder;
}

void mb_net_player_list_holder_serialize(MbNetPlayerListHolder * holder,
					 MbNetMessage * m)
{
	mb_net_message_add_int(m, g_list_length(holder->players));
	GList *next = holder->players;
	while (next != NULL) {
		MbNetPlayerHolder *h = (MbNetPlayerHolder *) next->data;
		mb_net_player_holder_serialize(h, m);
		next = g_list_next(next);
	}


}

void mb_net_player_list_holder_free(MbNetPlayerListHolder * holder)
{
}



MbNetScoreHolder *mb_net_score_holder_parse(MbNetMessage * m)
{
	MbNetScoreHolder *holder;
	holder = g_new0(MbNetScoreHolder, 1);
	guint32 size = mb_net_message_read_int(m);
	int i;
	for (i = 0; i < size; i++) {
		MbNetPlayerScoreHolder *h =
		    g_new0(MbNetPlayerScoreHolder, 1);
		h->name = mb_net_message_read_string(m);
		h->score = mb_net_message_read_int(m);
		holder->score_by_player =
		    g_list_append(holder->score_by_player, h);
	}
	return holder;
}

void mb_net_score_holder_serialize(MbNetScoreHolder * holder,
				   MbNetMessage * m)
{
	mb_net_message_add_int(m, g_list_length(holder->score_by_player));
	GList *next = holder->score_by_player;
	while (next != NULL) {
		MbNetPlayerScoreHolder *h =
		    (MbNetPlayerScoreHolder *) next->data;
		mb_net_message_add_string(m, h->name);
		mb_net_message_add_int(m, h->score);
		next = g_list_next(next);
	}
}

void mb_net_score_holder_free(MbNetScoreHolder * holder)
{
}


MbNetSimpleGameHolder *mb_net_simple_game_holder_create(guint handler_id,
							const gchar * name)
{
	MbNetSimpleGameHolder *h = g_new0(MbNetSimpleGameHolder, 1);
	h->handler_id = handler_id;
	h->name = g_strdup(name);
	return h;
}

MbNetGameListHolder *mb_net_game_list_holder_parse(MbNetMessage * m)
{

	MbNetGameListHolder *holder;
	holder = g_new0(MbNetGameListHolder, 1);
	gint size = mb_net_message_read_int(m);
	gint i;
	for (i = 0; i < size; i++) {
		MbNetSimpleGameHolder *h =
		    g_new0(MbNetSimpleGameHolder, 1);
		h->handler_id = mb_net_message_read_int(m);
		h->name = mb_net_message_read_string(m);
		holder->games = g_list_append(holder->games, h);
	}

	return holder;
}


void mb_net_game_list_holder_serialize(MbNetGameListHolder * holder,
				       MbNetMessage * m)
{
	mb_net_message_add_int(m, g_list_length(holder->games));
	GList *next = holder->games;
	while (next != NULL) {
		MbNetSimpleGameHolder *h;
		h = next->data;
		mb_net_message_add_int(m, h->handler_id);
		mb_net_message_add_string(m, h->name);
		next = g_list_next(next);
	}
}


void mb_net_game_list_holder_free(MbNetGameListHolder * holder)
{
}



void mb_net_simple_game_holder_free(MbNetSimpleGameHolder * holder)
{
	g_free(holder->name);
	g_free(holder);
}

MbNetPlayerHolder *mb_net_player_holder_parse(MbNetMessage * m)
{
	MbNetPlayerHolder *h;
	h = g_new0(MbNetPlayerHolder, 1);
	h->name = mb_net_message_read_string(m);
	h->player_id = mb_net_message_read_int(m);
	return h;
}

void mb_net_player_holder_serialize(MbNetPlayerHolder * holder,
				    MbNetMessage * m)
{
	mb_net_message_add_string(m, holder->name);
	mb_net_message_add_int(m, holder->player_id);
}


MbNetPlayerHolder *mb_net_player_holder_create(const gchar * name)
{
	MbNetPlayerHolder *holder =
	    (MbNetPlayerHolder *) g_new0(MbNetPlayerHolder, 1);
	holder->name = g_strdup(name);
	return holder;
}

void mb_net_player_holder_free(MbNetPlayerHolder * holder)
{
}

MbNetPlayerScoreHolder *mb_net_player_score_holder_create(const gchar *
							  name,
							  guint32 score)
{
	MbNetPlayerScoreHolder *holder = NULL;
	holder =
	    (MbNetPlayerScoreHolder *) g_new(MbNetPlayerScoreHolder, 1);
	holder->name = g_strdup(name);
	holder->score = score;
	return holder;

}

void mb_net_player_score_holder_free(MbNetPlayerScoreHolder * holder)
{
	g_free(holder->name);
	g_free(holder);
}
