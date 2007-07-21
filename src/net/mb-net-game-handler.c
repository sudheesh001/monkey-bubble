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

#include "mb-net-game-handler.h"

#include <glib.h>
#include <glib-object.h>

#include <net/monkey-net-marshal.h>

typedef struct _Private {
	int i;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	JOIN,
	JOIN_RESPONSE,
	ASK_PLAYER_LIST,
	PLAYER_LIST,
	MATCH_CREATED,
	START,
	STOP,
	ASK_SCORE,
	SCORE,
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_game_handler_get_property(GObject * object,
					     guint prop_id,
					     GValue * value,
					     GParamSpec * param_spec);
static void mb_net_game_handler_set_property(GObject * object,
					     guint prop_id,
					     const GValue * value,
					     GParamSpec * param_spec);

static void mb_net_game_handler_iface_init(MbNetHandlerInterface * iface);
static guint _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetGameHandler, mb_net_game_handler,
			MB_NET_TYPE_ABSTRACT_HANDLER, {
			G_IMPLEMENT_INTERFACE(MB_NET_TYPE_HANDLER,
					      mb_net_game_handler_iface_init)});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_GAME_HANDLER, Private))




static void mb_net_game_handler_finalize(MbNetGameHandler * self);

static void mb_net_game_handler_init(MbNetGameHandler * self);
static void _receive(MbNetHandler * handler, MbNetConnection * con,
		     guint32 i, guint32 j, guint32 k, MbNetMessage * m);



static void mb_net_game_handler_init(MbNetGameHandler * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
}


static void mb_net_game_handler_finalize(MbNetGameHandler * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}

static void
_receive(MbNetHandler * handler, MbNetConnection * con, guint32 src_id,
	 guint32 dest_id, guint32 action_id, MbNetMessage * m)
{
	MbNetGameHandler *self;
	Private *priv;
	self = MB_NET_GAME_HANDLER(handler);
	priv = GET_PRIVATE(self);

	guint32 code = action_id;


	if (code == JOIN) {
		guint32 player_id = mb_net_message_read_int(m);
		gboolean observer = mb_net_message_read_boolean(m);
		g_signal_emit(self, _signals[JOIN], 0, con,
			      src_id, player_id, observer);
	} else if (code == JOIN_RESPONSE) {
		gboolean observer = mb_net_message_read_boolean(m);
		gboolean master = mb_net_message_read_boolean(m);
		g_signal_emit(self, _signals[JOIN_RESPONSE], 0, con,
			      src_id, observer, master);
	} else if (code == ASK_PLAYER_LIST) {

		g_signal_emit(self, _signals[ASK_PLAYER_LIST], 0, con,
			      src_id);
	} else if (code == PLAYER_LIST) {
		MbNetPlayerListHolder *holder =
		    mb_net_player_list_holder_parse(m);
		g_signal_emit(self, _signals[PLAYER_LIST], 0, con, src_id,
			      holder);
		mb_net_player_list_holder_free(holder);
	} else if (code == MATCH_CREATED) {

		guint32 match_id = mb_net_message_read_int(m);
		gboolean observer = mb_net_message_read_boolean(m);
		g_signal_emit(self, _signals[MATCH_CREATED], 0, con,
			      src_id, match_id, observer);
	} else if (code == START) {
		g_signal_emit(self, _signals[START], 0, con, src_id);
	} else if (code == STOP) {
		g_signal_emit(self, _signals[STOP], 0, con, src_id);
	} else if (code == ASK_SCORE) {
		g_signal_emit(self, _signals[ASK_SCORE], 0, con, src_id);
	} else if (code == SCORE) {
		MbNetScoreHolder *holder = mb_net_score_holder_parse(m);
		g_signal_emit(self, _signals[SCORE], 0,
			      con, src_id, holder);
		mb_net_score_holder_free(holder);
	}
}


static guint32 _get_id(MbNetGameHandler * self)
{
	return mb_net_handler_get_id(MB_NET_HANDLER(self));
}

void mb_net_game_handler_send_join(MbNetGameHandler * self,
				   MbNetConnection * con,
				   guint32 handler_id, guint32 player_id,
				   gboolean has_observer)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, JOIN);
	mb_net_message_add_int(m, player_id);
	mb_net_message_add_boolean(m, has_observer);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void mb_net_game_handler_send_join_response(MbNetGameHandler * self,
					    MbNetConnection * con,
					    guint32 handler_id,
					    gboolean ok, gboolean master)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, JOIN_RESPONSE);
	mb_net_message_add_boolean(m, ok);
	mb_net_message_add_boolean(m, master);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void mb_net_game_handler_send_ask_player_list(MbNetGameHandler * self,
					      MbNetConnection * con,
					      guint32 handler_id)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, ASK_PLAYER_LIST);

	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void mb_net_game_handler_send_player_list(MbNetGameHandler * self,
					  MbNetConnection * con,
					  guint32 handler_id,
					  MbNetPlayerListHolder * holder)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, PLAYER_LIST);
	mb_net_player_list_holder_serialize(holder, m);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void mb_net_game_handler_send_match_created(MbNetGameHandler * self,
					    MbNetConnection * con,
					    guint32 handler_id,
					    guint32 match_id,
					    gboolean observer)
{

	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, MATCH_CREATED);
	mb_net_message_add_int(m, match_id);
	mb_net_message_add_boolean(m, observer);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}


void mb_net_game_handler_send_start(MbNetGameHandler * self,
				    MbNetConnection * con,
				    guint32 handler_id)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, START);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void mb_net_game_handler_send_stop(MbNetGameHandler * self,
				   MbNetConnection * con,
				   guint32 handler_id)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, STOP);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void mb_net_game_handler_send_ask_score(MbNetGameHandler * self,
					MbNetConnection * con,
					guint32 handler_id)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, ASK_SCORE);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}


void mb_net_game_handler_send_score(MbNetGameHandler * self,
				    MbNetConnection * con,
				    guint32 handler_id,
				    MbNetScoreHolder * holder)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, SCORE);
	mb_net_score_holder_serialize(holder, m);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

static void
mb_net_game_handler_get_property(GObject * object, guint prop_id,
				 GValue * value, GParamSpec * param_spec)
{
	MbNetGameHandler *self;

	self = MB_NET_GAME_HANDLER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_game_handler_set_property(GObject * object, guint prop_id,
				 const GValue * value,
				 GParamSpec * param_spec)
{
	MbNetGameHandler *self;

	self = MB_NET_GAME_HANDLER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}


static void mb_net_game_handler_iface_init(MbNetHandlerInterface * iface)
{
	iface->receive = _receive;
}


static void
mb_net_game_handler_class_init(MbNetGameHandlerClass *
			       mb_net_game_handler_class)
{
	GObjectClass *g_object_class;

	parent_class = g_type_class_peek_parent(mb_net_game_handler_class);


	g_type_class_add_private(mb_net_game_handler_class,
				 sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_game_handler_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_game_handler_set_property;
	g_object_class->get_property = mb_net_game_handler_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_game_handler_finalize;


	_signals[JOIN] = g_signal_new("join",
				      MB_NET_TYPE_GAME_HANDLER,
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET
				      (MbNetGameHandlerClass,
				       join),
				      NULL, NULL,
				      monkey_net_marshal_VOID__POINTER_UINT_UINT_UINT,
				      G_TYPE_NONE, 4,
				      G_TYPE_POINTER, G_TYPE_UINT,
				      G_TYPE_UINT, G_TYPE_UINT);
	_signals[JOIN_RESPONSE] =
	    g_signal_new("join-response", MB_NET_TYPE_GAME_HANDLER,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetGameHandlerClass,
					 join_response), NULL, NULL,
			 monkey_net_marshal_VOID__POINTER_UINT_UINT_UINT,
			 G_TYPE_NONE, 4, G_TYPE_POINTER, G_TYPE_UINT,
			 G_TYPE_UINT, G_TYPE_UINT);

	_signals[ASK_PLAYER_LIST] = g_signal_new("ask-player-list",
						 MB_NET_TYPE_GAME_HANDLER,
						 G_SIGNAL_RUN_LAST,
						 G_STRUCT_OFFSET
						 (MbNetGameHandlerClass,
						  ask_player_list),
						 NULL, NULL,
						 monkey_net_marshal_VOID__POINTER_UINT,
						 G_TYPE_NONE, 2,
						 G_TYPE_POINTER,
						 G_TYPE_UINT);


	_signals[PLAYER_LIST] = g_signal_new("player-list",
					     MB_NET_TYPE_GAME_HANDLER,
					     G_SIGNAL_RUN_LAST,
					     G_STRUCT_OFFSET
					     (MbNetGameHandlerClass,
					      player_list),
					     NULL, NULL,
					     monkey_net_marshal_VOID__POINTER_UINT_POINTER,
					     G_TYPE_NONE, 3,
					     G_TYPE_POINTER, G_TYPE_UINT,
					     G_TYPE_POINTER);

	_signals[MATCH_CREATED] = g_signal_new("match-created",
					       MB_NET_TYPE_GAME_HANDLER,
					       G_SIGNAL_RUN_LAST,
					       G_STRUCT_OFFSET
					       (MbNetGameHandlerClass,
						match_created),
					       NULL, NULL,
					       monkey_net_marshal_VOID__POINTER_UINT_UINT,
					       G_TYPE_NONE, 3,
					       G_TYPE_POINTER, G_TYPE_UINT,
					       G_TYPE_UINT);


	_signals[START] = g_signal_new("start",
				       MB_NET_TYPE_GAME_HANDLER,
				       G_SIGNAL_RUN_LAST,
				       G_STRUCT_OFFSET
				       (MbNetGameHandlerClass,
					start),
				       NULL, NULL,
				       monkey_net_marshal_VOID__POINTER_UINT,
				       G_TYPE_NONE, 2,
				       G_TYPE_POINTER, G_TYPE_UINT);


	_signals[STOP] = g_signal_new("stop",
				      MB_NET_TYPE_GAME_HANDLER,
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET
				      (MbNetGameHandlerClass,
				       stop),
				      NULL, NULL,
				      monkey_net_marshal_VOID__POINTER_UINT,
				      G_TYPE_NONE, 2,
				      G_TYPE_POINTER, G_TYPE_UINT);

	_signals[ASK_SCORE] = g_signal_new("ask-score",
					   MB_NET_TYPE_GAME_HANDLER,
					   G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET
					   (MbNetGameHandlerClass,
					    ask_score),
					   NULL, NULL,
					   monkey_net_marshal_VOID__POINTER_UINT,
					   G_TYPE_NONE, 2,
					   G_TYPE_POINTER, G_TYPE_UINT);

	_signals[SCORE] = g_signal_new("score",
				       MB_NET_TYPE_GAME_HANDLER,
				       G_SIGNAL_RUN_LAST,
				       G_STRUCT_OFFSET
				       (MbNetGameHandlerClass,
					score),
				       NULL, NULL,
				       monkey_net_marshal_VOID__POINTER_UINT_POINTER,
				       G_TYPE_NONE, 3,
				       G_TYPE_POINTER, G_TYPE_UINT,
				       G_TYPE_POINTER);
}
