/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* this file is part of criawips, a gnome presentation application
 *
 * AUTHORS
 *       Laurent Belmonte        <laurent.belmonte@aliacom.fr>
 *
 * Copyright (C) 2004 Laurent Belmonte
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

#ifndef PLAYER_INPUT_H
#define PLAYER_INPUT_H

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#define LEFT_KEY 1
#define RIGHT_KEY 2
#define SHOOT_KEY 3
G_BEGIN_DECLS

typedef struct MbPlayerInputPrivate MbPlayerInputPrivate;

typedef struct _MbPlayerInput MbPlayerInput;
typedef struct _MbPlayerInputClass MbPlayerInputClass;

MbPlayerInput *     mb_player_input_new                 (GtkWidget * window,
							 GConfClient * gconf_client,
							 const gchar * conf_key,
							 const gchar * player_name);

GType		mb_player_input_get_type	       (void);

#define MB_TYPE_PLAYER_INPUT			(mb_player_input_get_type ())
#define MB_PLAYER_INPUT(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_TYPE_PLAYER_INPUT, MbPlayerInput))
#define MB_PLAYER_INPUT_CLASS(klass)		(G_TYPE_CHACK_CLASS_CAST((klass), MB_TYPE_PLAYER_INPUT, MbPlayerInputClass))
#define MB_IS_PLAYER_INPUT(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_TYPE_PLAYER_INPUT))
#define MB_IS_PLAYER_INPUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_TYPE_PLAYER_INPUT))
#define MB_PLAYER_INPUT_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_TYPE_PLAYER_INPUT, MbPlayerInputClass))

struct _MbPlayerInput
{
	GObject		   base_instance;
	MbPlayerInputPrivate * private;
};

struct _MbPlayerInputClass
{
	GObjectClass	  base_class;
	
	// SIGNALS
	void (*input_notify_pressed) (MbPlayerInput * input,
				      gint key);


	void (*input_notify_released) (MbPlayerInput * input,
				       gint key);
};

G_END_DECLS

#endif /* PLAYER_INPUT_H */
