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

#ifndef _SHOOTER_H
#define _SHOOTER_H

#include <gtk/gtk.h>
#include "bubble.h"

G_BEGIN_DECLS

typedef struct ShooterPrivate ShooterPrivate;

typedef struct _Shooter Shooter;
typedef struct _ShooterClass ShooterClass;



GType shooter_get_type(void);


Shooter * shooter_new(gdouble x_pos,gdouble y_pos);

void shooter_add_bubble(Shooter * shooter,Bubble *b);

void shooter_get_position(const Shooter * shooter,gdouble * x,gdouble * y);

Bubble * shooter_shoot(Shooter * s);

Bubble * shooter_get_current_bubble(Shooter *s);
Bubble * shooter_get_waiting_bubble(Shooter *s);

gdouble shooter_get_angle(Shooter * s);

void shooter_set_angle(Shooter *s,gdouble angle);


#define TYPE_SHOOTER            (shooter_get_type())

#define SHOOTER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_SHOOTER,Shooter))
#define SHOOTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SHOOTER,ShooterClass))
#define IS_SHOOTER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_SHOOTER))
#define IS_SHOOTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SHOOTER))
#define SHOOTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SHOOTER, ShooterClass))

struct _Shooter {
	GObject parent_instance;
	ShooterPrivate * private;
};

struct _ShooterClass{
	GObjectClass parent_class;
	void (* rotated) (Shooter * shooter);
	void (* bubble_added) (Shooter * shooter,Bubble * b);
	void (* shoot) (Shooter * shooter,Bubble * b);
};
#endif
