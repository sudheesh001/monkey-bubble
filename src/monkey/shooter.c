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

#include <math.h>

#include "shooter.h"

static GObjectClass* parent_class = NULL;
#define PRIVATE(shooter) (shooter->private)

struct ShooterPrivate {
	gdouble x_pos;
	gdouble y_pos;
	gdouble angle;
	gdouble min_angle;
	gdouble max_angle;
	gdouble shoot_speed;
	Bubble * current_bubble;
	Bubble * waiting_bubble;
};

enum {
	ROTATED,
	BUBBLE_ADDED,
	SHOOT,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void shooter_notify_rotated(Shooter * self);
static void shooter_notify_bubble_added(Shooter * self,Bubble * bubble);

static void shooter_notify_shoot(Shooter * self,Bubble * b);

G_DEFINE_TYPE (Shooter, shooter, G_TYPE_OBJECT);

Shooter*
shooter_new(gdouble x_pos,gdouble y_pos)
{
	Shooter *self;

	self = SHOOTER( g_object_new(TYPE_SHOOTER,NULL));
  
	PRIVATE(self)->x_pos = x_pos;
	PRIVATE(self)->y_pos = y_pos;
	return self;
}


static void
shooter_init (Shooter* self)
{
	self->private = g_new0 (ShooterPrivate, 1);
	PRIVATE(self)->angle = 0;
	PRIVATE(self)->current_bubble = NULL;
	PRIVATE(self)->waiting_bubble = NULL;

	PRIVATE(self)->shoot_speed = 0.5;
	PRIVATE(self)->min_angle = -M_PI/2+0.01;
	PRIVATE(self)->max_angle = M_PI/2-0.01;

}

static void 
shooter_finalize(GObject* object) 
{
	Shooter * self = SHOOTER(object);


	if( PRIVATE(self)->current_bubble != NULL ) {
		g_object_unref( PRIVATE(self)->current_bubble );
	}

	if( PRIVATE(self)->waiting_bubble != NULL ) {
		g_object_unref( PRIVATE(self)->waiting_bubble );
	}

	g_free(self->private);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}






void 
shooter_add_bubble(Shooter * self,Bubble *b) 
{
	gdouble x,y;

	g_assert(IS_SHOOTER(self));
	g_assert(IS_BUBBLE(b));

	// FIXME ref b
	shooter_get_position( self, &x,&y );
  
	if( PRIVATE(self)->current_bubble == NULL) {
		bubble_set_position( b, x,y);
		PRIVATE(self)->current_bubble = b;
	} else {

		bubble_set_position( b, x - 55, y + 28 );
		PRIVATE(self)->waiting_bubble = b;
	}


	/* fire event */
	shooter_notify_bubble_added(self,b);
  
}


void
shooter_get_position(const Shooter * self,gdouble * x,gdouble * y)
{

	g_assert(IS_SHOOTER(self));
	*x = PRIVATE(self)->x_pos;
	*y = PRIVATE(self)->y_pos;
}


Bubble * 
shooter_get_current_bubble( Shooter *self) 
{

	g_assert( IS_SHOOTER( self ) );
	return PRIVATE(self)->current_bubble;
}

Bubble *
shooter_get_waiting_bubble(Shooter *self) 
{

	g_assert( IS_SHOOTER( self ) );

	return PRIVATE(self)->waiting_bubble;
}


gdouble 
shooter_get_angle(Shooter * self) 
{
	g_assert(IS_SHOOTER(self));
	return PRIVATE(self)->angle;
}


void 
shooter_set_angle(Shooter *self,gdouble angle) 
{
	gint test;
	g_assert(IS_SHOOTER(self));
  
	// FIXME this shit
	test = (gint32)rint(  angle*100000 );

	angle = ((gfloat)test)/100000;
	if( angle > PRIVATE(self)->max_angle ) {
		angle  = PRIVATE(self)->max_angle;
	}

	if( angle < PRIVATE(self)->min_angle ) {
		angle  = PRIVATE(self)->min_angle;
	}
  
	PRIVATE(self)->angle = angle;
  
	shooter_notify_rotated(self);
}


Bubble * 
shooter_shoot(Shooter * self) 
{
	Bubble * b;
	gdouble vx,vy;
	gdouble x,y;

	g_assert(IS_SHOOTER(self));
  
	g_assert( PRIVATE(self)->current_bubble != NULL ); 
  
	b = PRIVATE(self)->current_bubble;



	shooter_get_position( self, &x,&y );

	if( PRIVATE(self)->waiting_bubble != NULL) {
		bubble_set_position( PRIVATE(self)->waiting_bubble, x,y);
  
		PRIVATE(self)->current_bubble = PRIVATE(self)->waiting_bubble;
		PRIVATE(self)->waiting_bubble = NULL;
	}

	vx = - PRIVATE(self)->shoot_speed * sin( PRIVATE(self)->angle );
	vy = - PRIVATE(self)->shoot_speed * cos( PRIVATE(self)->angle );

	bubble_set_velocity( b, vx,vy);

	shooter_notify_shoot(self,b);
	return b;
}


static void 
shooter_notify_bubble_added(Shooter * self,Bubble * bubble) 
{
	
	g_signal_emit( G_OBJECT(self), signals[BUBBLE_ADDED],0,bubble);

}

static void 
shooter_notify_rotated(Shooter * self) 
{

	g_signal_emit( G_OBJECT(self), signals[ROTATED],0);
}

static void 
shooter_notify_shoot(Shooter * self,Bubble * b) 
{
	g_signal_emit( G_OBJECT(self), signals[SHOOT],0,b);  
}



static void
shooter_class_init (ShooterClass *klass) 
{
	GObjectClass* object_class;
  
	parent_class = g_type_class_peek_parent(klass);
	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = shooter_finalize;

	signals[ROTATED] = g_signal_new("rotated",				  
					G_TYPE_FROM_CLASS(klass),
					G_SIGNAL_RUN_FIRST |
					G_SIGNAL_NO_RECURSE,
					G_STRUCT_OFFSET (ShooterClass, rotated),
					NULL, NULL,
					g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 
					0,
					NULL);


	signals[BUBBLE_ADDED] = g_signal_new("bubble-added",				  
					     G_TYPE_FROM_CLASS(klass),
					     G_SIGNAL_RUN_FIRST |
					     G_SIGNAL_NO_RECURSE,
					     G_STRUCT_OFFSET (ShooterClass, bubble_added),
					     NULL, NULL,
					     g_cclosure_marshal_VOID__POINTER,
					     G_TYPE_NONE, 
					     1,
					     G_TYPE_POINTER);

	signals[SHOOT] = g_signal_new("shoot",				  
				      G_TYPE_FROM_CLASS(klass),
				      G_SIGNAL_RUN_FIRST |
				      G_SIGNAL_NO_RECURSE,
				      G_STRUCT_OFFSET (ShooterClass, shoot),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__POINTER,
				      G_TYPE_NONE, 
				      1,
				      G_TYPE_POINTER);
}

