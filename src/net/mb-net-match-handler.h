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
#include <net/mb-net-handler.h>
#include <monkey/color.h>
#ifndef _MB_NET__MATCH_HANDLER_H
#define _MB_NET__MATCH_HANDLER_H

G_BEGIN_DECLS typedef struct _MbNetMatchHandler MbNetMatchHandler;
typedef struct _MbNetMatchHandlerClass MbNetMatchHandlerClass;

GType mb_net_match_handler_get_type(void);

void mb_net_match_handler_send_penality(MbNetMatchHandler * self,
					MbNetConnection * con,
					guint32 handler_id, guint32 time,
					guint32 penality);
void mb_net_match_handler_send_next_row(MbNetMatchHandler * self,
					MbNetConnection * con,
					guint32 handler_id,
					Color * bubbles);
void mb_net_match_handler_send_new_cannon_bubble(MbNetMatchHandler * self,
						 MbNetConnection * con,
						 guint32 handler_id,
						 Color color);
void mb_net_match_handler_send_shoot(MbNetMatchHandler * self,
				     MbNetConnection * con,
				     guint32 handler_id, guint32 time,
				     gfloat radian);
void mb_net_match_handler_send_field(MbNetMatchHandler * self,
				     MbNetConnection * con,
				     guint32 handler_id, guint32 count,
				     Color * bubbles, gboolean odd);
void mb_net_match_handler_send_penality_bubbles(MbNetMatchHandler * self,
						MbNetConnection * con,
						guint32 handler_id,
						Color * bubbles);
void mb_net_match_handler_send_winlost(MbNetMatchHandler * self,
				       MbNetConnection * con,
				       guint32 handler_id, gboolean win);
void mb_net_match_handler_send_ready(MbNetMatchHandler * self,
				     MbNetConnection * con,
				     guint32 handler_id);
void mb_net_match_handler_send_start(MbNetMatchHandler * self,
				     MbNetConnection * con,
				     guint32 handler_id);

#define MB_NET_TYPE_MATCH_HANDLER			(mb_net_match_handler_get_type())
#define MB_NET_MATCH_HANDLER(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_NET_TYPE_MATCH_HANDLER, MbNetMatchHandler))
#define MB_NET_MATCH_HANDLER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MB_NET_TYPE_MATCH_HANDLER, MbNetMatchHandlerClass))
#define MB_NET_IS_MATCH_HANDLER(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_NET_TYPE_MATCH_HANDLER))
#define MB_NET_IS_MATCH_HANDLER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_NET_TYPE_MATCH_HANDLER))
#define MB_NET_MATCH_HANDLER_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_NET_TYPE_MATCH_HANDLER, MbNetMatchHandlerClass))

struct _MbNetMatchHandler {
	GObject base_instance;
};

struct _MbNetMatchHandlerClass {
	GObjectClass base_class;

	/* signals */
	void (*penality) (MbNetMatchHandler * self, MbNetConnection * con,
			  guint32 handler_id, guint32 time,
			  guint32 penality);
	void (*next_row) (MbNetMatchHandler * self, MbNetConnection * con,
			  guint32 handler_id, Color * bubbles);
	void (*new_cannon_bubble) (MbNetMatchHandler * self,
				   MbNetConnection * con,
				   guint32 handler_id, Color color);
	void (*shoot) (MbNetMatchHandler * self, MbNetConnection * con,
		       guint32 handler_id, guint32 time, gfloat radian);
	void (*field) (MbNetMatchHandler * self, MbNetConnection * con,
		       guint32 handler_id, guint32 count, Color * bubbles,
		       gboolean odd);
	void (*penality_bubbles) (MbNetMatchHandler * self,
				  MbNetConnection * con,
				  guint32 handler_id, Color * bubbles);
	void (*winlost) (MbNetMatchHandler * self, MbNetConnection * con,
			 guint32 handler_id, gboolean win);
	void (*ready) (MbNetMatchHandler * self, MbNetConnection * con,
		       guint32 handler_id);
	void (*start) (MbNetMatchHandler * self, MbNetConnection * con,
		       guint32 handler_id);

};

G_END_DECLS
#endif				/* !_MB_NET::_MATCH_HANDLER_H */
