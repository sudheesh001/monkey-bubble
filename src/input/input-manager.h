/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
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

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <gtk/gtk.h>

#include "player-input.h"
#define CONF_KEY_PREFIX "/apps/monkey-bubble"
#define CONF_GLOBAL_PREFIX "/apps/monkey-bubble"

G_BEGIN_DECLS

typedef struct MbInputManagerPrivate MbInputManagerPrivate;

typedef struct _MbInputManager MbInputManager;
typedef struct _MbInputManagerClass MbInputManagerClass;

MbInputManager * mb_input_manager_get_instance(void);
void mb_input_manager_instance_set_window(GtkWidget * window);

MbPlayerInput * mb_input_manager_get_left(MbInputManager * m);
MbPlayerInput * mb_input_manager_get_right(MbInputManager * m);

GType		mb_input_manager_get_type	       (void);

#define MB_TYPE_INPUT_MANAGER			(mb_input_manager_get_type ())
#define MB_INPUT_MANAGER(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_TYPE_INPUT_MANAGER, MbInputManager))
#define MB_INPUT_MANAGER_CLASS(klass)		(G_TYPE_CHACK_CLASS_CAST((klass), MB_TYPE_INPUT_MANAGER, MbInputManagerClass))
#define MB_IS_INPUT_MANAGER(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_TYPE_INPUT_MANAGER))
#define MB_IS_INPUT_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_TYPE_INPUT_MANAGER))
#define MB_INPUT_MANAGER_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_TYPE_INPUT_MANAGER, MbInputManagerClass))

struct _MbInputManager
{
	GObject		   base_instance;
	MbInputManagerPrivate * private;
};

struct _MbInputManagerClass
{
	GObjectClass	  base_class;
};

G_END_DECLS

#endif /* INPUT_MANAGER_H */
