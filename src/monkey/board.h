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
#ifndef _BOARD_H
#define _BOARD_H

#include <gtk/gtk.h>
#include "bubble.h"


typedef struct BoardPrivate BoardPrivate;

typedef struct _Board Board;
typedef struct _BoardClass BoardClass;


Board *board_new (gdouble y_min, const gchar * filename, gint level);

void board_init (Board * board, Bubble ** bubbles, gint count);

void board_add_bubbles (Board * board, Bubble ** bubbles);

void board_insert_bubbles (Board * board, Bubble ** bubbles);

Bubble *board_get_bubble_at (Board * board, gint x, gint y);

gint *board_get_colors_count (Board * b);
gint board_get_row_count (Board * b);
gint board_get_column_count (Board * b);

void board_stick_bubble (Board * board, Bubble * bubble, gint time);
gboolean board_collide_bubble (Board * board, Bubble * b);
gboolean board_is_lost (Board * board);

gdouble board_get_y_min (Board * board);

void board_down (Board * board);

gboolean board_get_odd(Board * board);
int board_bubbles_count (Board * board);

Bubble **board_get_array (Board * board);
void board_print (Board * board);


#define TYPE_BOARD            (board_get_type())

#define BOARD(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_BOARD,Board))
#define BOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_BOARD,BoardClass))
#define IS_BOARD(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_BOARD))
#define IS_BOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_BOARD))
#define BOARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_BOARD, BoardClass))

struct _Board
{
	GObject parent_instance;
	BoardPrivate *private;
};

struct _BoardClass
{
	GObjectClass parent_class;
	void (*bubbles_exploded) (Board * board, GList * exploded,
				  GList * fallen);

	void (*bubbles_added) (Board * board, GList * bubbles);

	void (*bubble_sticked) (Board * board, Bubble * bubble, gint stiked);


	void (*bubbles_inserted) (Board * board, Bubble ** bubbles,
				  int count);


	void (*down) (Board * board);

};

GType board_get_type (void);

typedef struct
{
	gint x;
	gint y;
} Cell_xy;
#endif
