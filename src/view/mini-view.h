/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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

#ifndef MINI_VIEW_H
#define MINI_VIEW_H

#include <glib-object.h>

#include "monkey-canvas.h"
#include "color.h"

G_BEGIN_DECLS

typedef struct MbMiniViewPrivate MbMiniViewPrivate;

typedef struct _MbMiniView MbMiniView;
typedef struct _MbMiniViewClass MbMiniViewClass;

MbMiniView *     mb_mini_view_new                 (MonkeyCanvas * canvas,
						   gint x,gint y);

void mb_mini_view_update(MbMiniView * self,
			 Color * bubbles,
			 int odd);

void mb_mini_view_lost(MbMiniView * self);
void mb_mini_view_win(MbMiniView * self);
void mb_mini_view_set_score(MbMiniView * self,guint score);
GType		mb_mini_view_get_type	       (void);

#define MB_TYPE_MINI_VIEW			(mb_mini_view_get_type ())
#define MB_MINI_VIEW(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_TYPE_MINI_VIEW, MbMiniView))
#define MB_MINI_VIEW_CLASS(klass)		(G_TYPE_CHACK_CLASS_CAST((klass), MB_TYPE_MINI_VIEW, MbMiniViewClass))
#define MB_IS_MINI_VIEW(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_TYPE_MINI_VIEW))
#define MB_IS_MINI_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_TYPE_MINI_VIEW))
#define MB_MINI_VIEW_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_TYPE_MINI_VIEW, MbMiniViewClass))

struct _MbMiniView
{
	GObject		   base_instance;
	MbMiniViewPrivate * private;
};

struct _MbMiniViewClass
{
	GObjectClass	  base_class;
};

G_END_DECLS

#endif /* MINI_VIEW_H */
