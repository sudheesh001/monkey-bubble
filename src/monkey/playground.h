/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* this file is part of monkey bubble a gnome game
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
#ifndef _PLAYGROUND_H
#define _PLAYGROUND_H

#include <gtk/gtk.h>

#include "bubble.h"
#include "board.h"


G_BEGIN_DECLS

typedef struct PlaygroundPrivate PlaygroundPrivate;

typedef struct _Playground Playground;
typedef struct _PlaygroundClass PlaygroundClass;



Playground * playground_new(gdouble max_x,gdouble min_x,
			    const gchar * filename,gint level);

Bubble * playground_get_active_bubble(Playground * playground);

gboolean playground_is_ready_for_shoot(Playground * playground);

Board * playground_get_board(Playground * playground );

void playground_update(Playground * playground,gint time);

void playground_shoot_bubble(Playground *pl,Bubble * b);

#ifdef MAEMO
void playground_save(Playground * self, const gchar * level_filename);
#endif

#define TYPE_PLAYGROUND            (playground_get_type())

#define PLAYGROUND(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_PLAYGROUND,Playground))
#define PLAYGROUND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_PLAYGROUND,PlaygroundClass))
#define IS_PLAYGROUND(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_PLAYGROUND))
#define IS_PLAYGROUND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_PLAYGROUND))
#define PLAYGROUND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PLAYGROUND, PlaygroundClass))


struct _Playground {
	GObject parent_instance;
	PlaygroundPrivate * private;
};

struct _PlaygroundClass {
	GObjectClass parent_class;
	void (* bubble_shot )( Playground *pg,
			       Bubble * b);
    
	void (* bubble_wall_collision )(Playground *pg);

	void (* bubble_board_collision )(Playground *pg);

	void (* game_lost)(Playground * pg);

};

GType playground_get_type(void);
#endif
