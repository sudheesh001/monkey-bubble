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
#include "playground.h"
#define PRIVATE(playground) (playground->private)


static GObjectClass* parent_class = NULL;

enum {
	BUBBLE_SHOT,
	BUBBLE_WALL_COLLISION,
	BUBBLE_BOARD_COLLISION,
	GAME_LOST,
	LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL];

struct PlaygroundPrivate {
	Board * board;
	Bubble * played_bubble;
	gdouble max_x;
	gdouble min_x;
	gint time;
};


static void playground_notify_bubble_wall_collision(Playground * p);
static void playground_notify_lost(Playground * p);



Playground * 
playground_new(gdouble max_x,gdouble min_x,const gchar * level_filename,
	       gint level) 

{
	Playground * self;
	self =  PLAYGROUND( g_object_new(TYPE_PLAYGROUND,NULL));

	PRIVATE(self)->max_x = max_x;
	PRIVATE(self)->min_x = min_x;
	PRIVATE(self)->board =  board_new( 40,level_filename,level );
	return self;
}


static void 
playground_instance_init(Playground * self) 
{
	self->private =g_new0 (PlaygroundPrivate, 1);			
	PRIVATE(self)->played_bubble = NULL;
	PRIVATE(self)->time = 0;
}

static void 
playground_finalize(GObject* object) 
{
	Playground * self = PLAYGROUND(object);


	g_object_unref( PRIVATE(self)->board );

	if( PRIVATE(self)->played_bubble != NULL ) {
		g_object_unref( PRIVATE(self)->played_bubble );
	}

	g_free(self->private);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}




Bubble * 
playground_get_active_bubble(Playground * self) 
{
	g_assert( IS_PLAYGROUND( self ));
	return PRIVATE(self)->played_bubble;
}


Board * 
playground_get_board(Playground * self ) 
{
	g_assert( IS_PLAYGROUND( self ));
	return PRIVATE(self)->board;
}


/*
 * The playground is ready to receive a shoot only
 * if it hasnt an active bubble
 */
gboolean 
playground_is_ready_for_shoot(Playground * self) 
{

	g_assert( IS_PLAYGROUND( self ));
	return ( PRIVATE(self)->played_bubble == NULL );
}


static void 
playground_notify_bubble_wall_collision(Playground * self) 
{

	g_assert( IS_PLAYGROUND( self ));

	g_signal_emit( self, signals[BUBBLE_WALL_COLLISION],0);


}

static void 
playground_notify_lost(Playground * self) 
{

	g_assert( IS_PLAYGROUND( self ));

	g_signal_emit( self, signals[GAME_LOST],0);


}

void
playground_update(Playground * self,gint time) 
{
	Bubble * bubble;
	g_assert( IS_PLAYGROUND( self ));
  
	PRIVATE(self)->time += time;
	bubble = PRIVATE(self)->played_bubble;
  
	if( bubble != NULL ) {
		  
		gdouble x,y,vx,vy;   
		  
		bubble_get_position(bubble,&x,&y);    
		bubble_get_velocity(bubble,&vx,&vy);
    
		x += time * vx;
		y += time * vy;
    
		if( ( x - BUBBLE_RADIUS) < PRIVATE(self)->min_x ) {
			x = 
				PRIVATE(self)->min_x +
				BUBBLE_RADIUS +
				( PRIVATE(self)->min_x -
				  ( x - BUBBLE_RADIUS ) );
			vx = - vx;
			playground_notify_bubble_wall_collision(self);
		}
		  
		if( (x+BUBBLE_RADIUS) > PRIVATE(self)->max_x) {
			x =
				PRIVATE(self)->max_x - 
				BUBBLE_RADIUS +
				( PRIVATE(self)->max_x - 
				  ( x + BUBBLE_RADIUS ));
			vx = - vx;
			playground_notify_bubble_wall_collision(self);
		}
		  
		bubble_set_velocity(bubble,vx,vy);
		  
		bubble_set_position(bubble,x,y);
		  
		if( board_collide_bubble( PRIVATE(self)->board, bubble ) ) {
			g_signal_emit( self, signals[BUBBLE_BOARD_COLLISION],0);

      
			board_stick_bubble(PRIVATE(self)->board,PRIVATE(self)->played_bubble,PRIVATE(self)->time);
			PRIVATE(self)->played_bubble = NULL;
			if( board_is_lost( PRIVATE(self)->board) ) {
				playground_notify_lost(self);
			}
		}
	}
  
}

void
playground_shoot_bubble(Playground *self,Bubble * bubble) 
{
  
	g_assert( IS_PLAYGROUND( self ));
  
	PRIVATE(self)->played_bubble = bubble;
  

	g_signal_emit( G_OBJECT(self), signals[BUBBLE_SHOT],0,bubble);
}


static void playground_class_init (PlaygroundClass *klass) {
	GObjectClass* object_class;
	parent_class = g_type_class_peek_parent(klass);
	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = playground_finalize;


	signals[BUBBLE_SHOT] = 
		g_signal_new( "bubble-shot",
			      G_TYPE_FROM_CLASS(klass),
			      G_SIGNAL_RUN_FIRST |
			      G_SIGNAL_NO_RECURSE,
			      G_STRUCT_OFFSET (PlaygroundClass,
					       bubble_shot),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 
			      1,
			      G_TYPE_POINTER);


	signals[BUBBLE_WALL_COLLISION] = 
		g_signal_new( "bubble-wall-collision",
			      G_TYPE_FROM_CLASS(klass),
			      G_SIGNAL_RUN_FIRST |
			      G_SIGNAL_NO_RECURSE,
			      G_STRUCT_OFFSET (PlaygroundClass,
					       bubble_wall_collision),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0,
			      NULL);

	signals[BUBBLE_BOARD_COLLISION] = 
		g_signal_new( "bubble-board-collision",
			      G_TYPE_FROM_CLASS(klass),
			      G_SIGNAL_RUN_FIRST |
			      G_SIGNAL_NO_RECURSE,
			      G_STRUCT_OFFSET (PlaygroundClass,
					       bubble_board_collision),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0,
			      NULL);

	signals[GAME_LOST] = 
		g_signal_new( "game-lost",
			      G_TYPE_FROM_CLASS(klass),
			      G_SIGNAL_RUN_FIRST |
			      G_SIGNAL_NO_RECURSE,
			      G_STRUCT_OFFSET (PlaygroundClass,
					       game_lost),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0,
			      NULL);
    
    
}


GType playground_get_type(void) {
	static GType playground_type = 0;
    
	if (!playground_type) {
		static const GTypeInfo playground_info = {
			sizeof(PlaygroundClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) playground_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof(Playground),
			1,              /* n_preallocs */
			(GInstanceInitFunc) playground_instance_init,
		};


      
		playground_type = g_type_register_static(G_TYPE_OBJECT,
							 "Playground",
							 &playground_info, 0);
	}
    
	return playground_type;
}

