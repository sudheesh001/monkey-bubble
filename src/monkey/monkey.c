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
#include <gtk/gtk.h>
#include "monkey.h"
#include <math.h>
#include <stdlib.h>
#include "monkey-marshal.h"

#define PRIVATE(monkey) (monkey->private )
#define MAX_UPDATE_TIME 1

static GObjectClass* parent_class = NULL;

enum {
	GAME_LOST,
	SHOOTER_UP,
	SHOOTER_DOWN,
	SHOOTER_CENTER,
	BUBBLES_EXPLODED,
	BUBBLE_SHOT,
	BUBBLE_STICKED,
	BUBBLES_WAITING_CHANGED,
	HURRY_UP,
	LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL];

struct MonkeyPrivate {
	Playground * playground;
	Shooter * shooter;  
	gint left_pressed;
	gint left_pressed_time;
	gint right_pressed;
	gint right_pressed_time;
	gint time;
	gint last_shoot;
	gint last_stiked;
	gint shot_count;
	GList * to_add;
	gboolean hurry_up;
	gboolean network;
};

static void monkey_finalize(GObject* object);

static void monkey_update_shooter(Monkey * self,gint time);
static void monkey_add_waiting_row(Monkey * self);


static void monkey_bubble_sticked ( Board *board,Bubble * bubble,
				    gint time,
				    Monkey * self);

static void monkey_bubbles_exploded ( Board *board,
				      GList * exploded,
				      GList * fallen,
				      Monkey * self);


static void monkey_add_new_waiting_row(Monkey * self);
static void monkey_playground_lost(Playground * pg,Monkey * self);

static void monkey_playground_shot(Playground *pg,Bubble * b,Monkey * self);

static void monkey_notify_game_lost(Monkey * self);
static void monkey_notify_bubble_shot(Monkey * self,Bubble * b);
static void monkey_notify_bubbles_exploded(Monkey * self,
					   GList * exploded,
					   GList * fallen);


static gboolean monkey_has_waiting_row(Monkey * self);

static void 
monkey_instance_init(Monkey * self) 
{
	self->private =g_new0 (MonkeyPrivate, 1);			

	PRIVATE(self)->shot_count = 0;

	PRIVATE(self)->left_pressed = 0;
	PRIVATE(self)->left_pressed_time = 0;
	PRIVATE(self)->right_pressed = 0;
	PRIVATE(self)->left_pressed_time = 0;
	PRIVATE(self)->time = 0;

	PRIVATE(self)->last_stiked = 0;

	PRIVATE(self)->network = FALSE;

	PRIVATE(self)->to_add = NULL;

	PRIVATE(self)->hurry_up = TRUE;

}

/**
 * construct model from file
 */
Monkey * 
monkey_new_level_from_file(const gchar * filename,int level) 
{
	Monkey * self;
	Board * board;
	self = MONKEY (g_object_new (TYPE_MONKEY, NULL));


	PRIVATE(self)->playground = playground_new( 446,190,filename,level);
  
	PRIVATE(self)->shooter = shooter_new( 318,400);

  
	board =  playground_get_board(PRIVATE(self)->playground);
	g_signal_connect( board,"bubbles-exploded",
			  G_CALLBACK(monkey_bubbles_exploded),self);


	g_signal_connect( board,"bubble-sticked",
			  G_CALLBACK(monkey_bubble_sticked),self);

	g_signal_connect( PRIVATE(self)->playground,"bubble-shot",
			  G_CALLBACK(monkey_playground_shot),self);

	g_signal_connect( PRIVATE(self)->playground,"game-lost",
			  G_CALLBACK(monkey_playground_lost),self);
	return self;
}

Monkey * 
monkey_new(gboolean network) 
{
	Monkey * self;
	self = monkey_new_level_from_file(NULL,-1);
	PRIVATE(self)->network = network;
	return self;
}


static void free_to_add(Monkey * self) 
{
	GList * next;
	 
	next = PRIVATE(self)->to_add;
	 

	while( next != NULL ) {
		int i;
		Bubble ** bubbles;

		bubbles = (Bubble **) next->data;
		  
		for(i=0;i < 7; i++ ) {
			if( bubbles[i] != NULL ) {
				g_object_unref(bubbles[i]);
			}
		}
		g_free(next->data);
		next = g_list_next(next);
	}
  
	g_list_free( PRIVATE(self)->to_add);

	PRIVATE(self)->to_add = NULL;

}

static void 
monkey_finalize(GObject* object)
{
	Monkey * self = MONKEY(object);
	 
	 

	g_signal_handlers_disconnect_matched( PRIVATE(self)->playground,
					      G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,self);


	g_signal_handlers_disconnect_matched( playground_get_board(PRIVATE(self)->playground),
					      G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,self);
	 
	g_object_unref( PRIVATE(self)->playground);
	g_object_unref( PRIVATE(self)->shooter);
	 

	free_to_add(self);
	 
	g_free(self->private);
	 
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}


void
monkey_left_changed( Monkey * self,gboolean pressed,gint time) 
{

	g_assert( IS_MONKEY( self ) );

	if( pressed ) {

		if( PRIVATE(self)->left_pressed != 0 ) return;
		PRIVATE(self)->left_pressed = time;

	} else {
		PRIVATE(self)->left_pressed_time += 
			time - PRIVATE(self)->left_pressed;
  
		PRIVATE(self)->left_pressed = 0;
	}

}

void
monkey_shoot(Monkey * self,gint time) 
{
	g_assert( IS_MONKEY( self ) );


	monkey_update_shooter( self, time );

	if( playground_is_ready_for_shoot( PRIVATE(self)->playground) &&
	    (abs( time - PRIVATE(self)->last_shoot) > 500) ) {

		PRIVATE(self)->last_shoot = time;
		PRIVATE(self)->last_stiked = time;

		playground_shoot_bubble( PRIVATE(self)->playground,
					 shooter_shoot( PRIVATE(self)->shooter));
		PRIVATE(self)->shot_count++;
	}
}

void 
monkey_right_changed( Monkey * self,gboolean pressed,gint time)
{
	 
	g_assert( IS_MONKEY( self ) );

	if( pressed ) {

		if( PRIVATE(self)->right_pressed != 0 ) return;
		PRIVATE(self)->right_pressed = time;

	} else {
		PRIVATE(self)->right_pressed_time += time -
			PRIVATE(self)->right_pressed;

		PRIVATE(self)->right_pressed = 0;
	}

}


static void 
monkey_update_shooter(Monkey * self,gint time)
{

	gdouble new_angle;

	if( PRIVATE(self)->right_pressed != 0 ) {
		PRIVATE(self)->right_pressed_time += time -
			PRIVATE(self)->right_pressed;

		PRIVATE(self)->right_pressed = time;
    
	}



	if( PRIVATE(self)->left_pressed != 0 ) {
		PRIVATE(self)->left_pressed_time += time -
			PRIVATE(self)->left_pressed;

		PRIVATE(self)->left_pressed = time;
    
	}

	if( PRIVATE(self)->left_pressed!= 0 ) {
		g_signal_emit( G_OBJECT(self),signals[SHOOTER_UP],0);

	} else if( PRIVATE(self)->right_pressed != 0 ) {
		g_signal_emit( G_OBJECT(self),signals[SHOOTER_DOWN],0);

	} else {
		g_signal_emit( G_OBJECT(self),signals[SHOOTER_CENTER],0);

	}



	if( ( PRIVATE(self)->left_pressed_time != 0) ||
	    ( PRIVATE(self)->right_pressed_time != 0 ) ) {

		new_angle =
			(((gdouble)
			  ( PRIVATE(self)->left_pressed_time - PRIVATE(self)->right_pressed_time))
			 / 2000)*M_PI
			+
			shooter_get_angle(PRIVATE(self)->shooter);


		shooter_set_angle( PRIVATE(self)->shooter,
				   new_angle
				   );

		PRIVATE(self)->left_pressed_time = 0;
		PRIVATE(self)->right_pressed_time =0;
	}

}

void 
monkey_update( Monkey * self,gint time ) 
{

	g_assert( IS_MONKEY( self ) );

	monkey_update_shooter(self,time);


  
	while ( ( time - PRIVATE( self )->time ) != 0 ) {


		if( ( time - PRIVATE( self )->time ) > MAX_UPDATE_TIME ) {
			playground_update( PRIVATE( self )->playground,
					   MAX_UPDATE_TIME );
    
			PRIVATE( self )->time += MAX_UPDATE_TIME;  

		} else {

			playground_update( PRIVATE( self )->playground,
					   ( time - PRIVATE( self )->time ) );
    
			PRIVATE( self )->time = time;  

		}
  
	}
	PRIVATE( self )->time = time;

    

	if( (time - PRIVATE(self)->last_stiked) > 10000) { 
		if( ! PRIVATE(self)->network) {
			monkey_shoot(self,time);
		}
	} else if( ( time - PRIVATE(self)->last_stiked) > 7000 &&
		   PRIVATE(self)->hurry_up ) {
		g_signal_emit( G_OBJECT(self),signals[HURRY_UP],0);
		PRIVATE(self)->hurry_up = FALSE;
	}
}


Shooter * 
monkey_get_shooter(Monkey * self) 
{
	g_assert( IS_MONKEY( self ) );

	return PRIVATE(self)->shooter;
}


gint
monkey_get_shot_count(Monkey * self) 
{
	g_assert( IS_MONKEY( self ) );

	return PRIVATE(self)->shot_count;
}

Playground *
monkey_get_playground(Monkey * self) 
{
	g_assert( IS_MONKEY( self ) );

	return PRIVATE(self)->playground;
}
  

static void 
monkey_notify_bubbles_waiting_changed(Monkey * self) {

	GList * next;
	Bubble ** bubbles;
	int i;
	int count;

	count = 0;
  
	next = PRIVATE(self)->to_add;
	while( next != NULL ) {
		bubbles = (Bubble **)next->data;
    
		for(i= 0; i < 7 ;i++) {
			if( bubbles[i] != NULL ) {
				count++;
			}
		}

		next = g_list_next(next);
	}



	g_signal_emit( G_OBJECT(self),signals[BUBBLES_WAITING_CHANGED],0,count);
  

}

																  
static void 
monkey_notify_bubbles_exploded(Monkey * self,
			       GList * exploded,
			       GList * fallen) 
{
	g_signal_emit( G_OBJECT(self),signals[BUBBLES_EXPLODED],0,exploded,fallen);
}


static void 
monkey_notify_bubble_shot(Monkey * self,Bubble * b) 
{
	g_signal_emit( G_OBJECT(self),signals[BUBBLE_SHOT],0,b);
}

static void 
monkey_notify_bubble_sticked(Monkey * self,Bubble * b) 
{
	g_signal_emit( G_OBJECT(self),signals[BUBBLE_STICKED],0,b);
}

static void 
monkey_notify_game_lost(Monkey * self) 
{
	g_signal_emit( G_OBJECT(self),signals[GAME_LOST],0);
}

static void
monkey_playground_shot(Playground *pg,Bubble * b,Monkey * self) 
{
	g_assert( IS_MONKEY(self) );


	monkey_notify_bubble_shot(self,b);
  

}


static void 
monkey_playground_lost(Playground * pg,
		       Monkey * self) 
{
	 
	g_assert( IS_MONKEY(self) );
	 
	monkey_notify_game_lost(self);
	 
}


static void
monkey_bubble_sticked (Board * board,Bubble * bubble,gint time,Monkey * self) 
{

	g_assert( IS_MONKEY(self) );

	PRIVATE(self)->hurry_up = TRUE;

	monkey_add_waiting_row(self);
	monkey_notify_bubbles_waiting_changed(self);
	PRIVATE(self)->last_stiked = time;
	monkey_notify_bubble_sticked( self,bubble );

}


static void 
monkey_bubbles_exploded (Board *board,
			 GList * exploded,
			 GList * fallen,
			 Monkey * self) 
{

	g_assert( IS_MONKEY(self) );
	g_assert( IS_BOARD(board));

	monkey_notify_bubbles_exploded(self,
				       exploded,
				       fallen);
  
}



Bubble **
monkey_get_current_free_columns(Monkey * self) 
{
	if( PRIVATE(self)->to_add != NULL ) {
		return (Bubble **)g_list_last(PRIVATE(self)->to_add)->data;
	} else return NULL;

}

guint8 *
monkey_add_bubbles(Monkey * self,
		   int bubbles_count,
		   Color * bubbles_colors ) 
{


	guint8 * columns;
	int empty_column_count,c,i,index;
	Bubble ** bubbles;
  

	g_assert( IS_MONKEY( self ));
  
	bubbles = monkey_get_current_free_columns(self);

  
	columns = g_malloc( sizeof(guint8)*bubbles_count);
  
	/* count the empty columns */
	empty_column_count = 0;

  
	if( bubbles != NULL) {
		for( c = 0; c < 7; c++) {
			if( bubbles[c] == NULL) {
				empty_column_count++;
			} 
		}
	}

	index = 0;
	while( index < bubbles_count  ) {
		
		if( empty_column_count == 0 ) {
			empty_column_count = 7;
			monkey_add_new_waiting_row(self);
			bubbles = monkey_get_current_free_columns(self);
		}

		c = rand()% empty_column_count;

		for( i = 0; i < 7; i++) {


			if( (c <= 0) && ( bubbles[i] == NULL) ) {
				g_assert(bubbles[i] == NULL );
				bubbles[ i ] = bubble_new( bubbles_colors[ index ],0,0 );
				columns[index] = i;
				index++;
				empty_column_count--;
				break;
			}

			if( bubbles[i] == NULL ) c--;


		}

	 
	}

	monkey_notify_bubbles_waiting_changed( self);
	return columns;
}

void
monkey_add_bubbles_at (Monkey * self,
		       int bubbles_count,
		       Color * bubbles_colors,
		       guint8 * bubbles_column )  
{

	Bubble ** bubbles;
	int i;
	g_assert( IS_MONKEY( self ));



	bubbles = monkey_get_current_free_columns(self);

	if( bubbles == NULL) {
		monkey_add_new_waiting_row(self);
		bubbles = monkey_get_current_free_columns(self);		  
	}

	for( i = 0; i < bubbles_count; i++ ) {
		if(bubbles[ bubbles_column[i]] != NULL) {
			 
			monkey_add_new_waiting_row(self);
			bubbles = monkey_get_current_free_columns(self);

		}
		bubbles[ bubbles_column[i] ] = bubble_new( bubbles_colors[i],0,0 );
	}

	monkey_notify_bubbles_waiting_changed( self);

}

static gboolean 
monkey_has_waiting_row(Monkey * self) 
{
 
	if( PRIVATE(self)->to_add == NULL ) return FALSE;
	else return TRUE;
}

static void
monkey_add_new_waiting_row(Monkey * self) 
{
	Bubble ** row_to_add;
	int i;
	 
	/* have to add a row */
	row_to_add = g_malloc( sizeof(Bubble *) * 7 );
	 
	for(i = 0; i < 7; i++) {
		row_to_add[i] = NULL;
	}
	 
	PRIVATE(self)->to_add = g_list_append( PRIVATE(self)->to_add,
					       row_to_add);
	 
}

static void 
monkey_add_waiting_row(Monkey * self) 
{


	if( monkey_has_waiting_row(self) ) {
		Bubble ** bubbles;

		bubbles = (Bubble **)PRIVATE(self)->to_add->data;

		PRIVATE(self)->to_add =
			g_list_remove(PRIVATE(self)->to_add,
				      bubbles);
    
		board_add_bubbles( playground_get_board(PRIVATE(self)->playground),
				   bubbles );
    
    
	 
	}

}

void 
monkey_set_board_down(Monkey * self) 
{
	g_assert( IS_MONKEY(self));

	board_down( playground_get_board( PRIVATE(self)->playground ));
}


void
monkey_insert_bubbles(Monkey * self,Bubble ** bubbles_8) 
{
	g_assert( IS_MONKEY( self ) );
	 
	board_insert_bubbles( playground_get_board( PRIVATE(self)->playground),
			      bubbles_8);
}


gboolean
monkey_is_empty(Monkey * self) 
{
	 
	g_assert( IS_MONKEY(self) );

	return ( board_bubbles_count(playground_get_board(PRIVATE(self)->playground)) == 0 );
}

void
monkey_print_board(Monkey * self) 
{
	board_print( playground_get_board(PRIVATE(self)->playground));
}



static void
monkey_class_init (MonkeyClass *klass)
{
	GObjectClass* object_class;

	parent_class = g_type_class_peek_parent(klass);
	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = monkey_finalize;

	signals[GAME_LOST]= g_signal_new ("game-lost",
					  G_TYPE_FROM_CLASS (klass),
					  G_SIGNAL_RUN_FIRST |
					  G_SIGNAL_NO_RECURSE,
					  G_STRUCT_OFFSET (MonkeyClass, game_lost),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID,
					  G_TYPE_NONE,
					  0,NULL);


	signals[SHOOTER_UP]= g_signal_new ("up",
					   G_TYPE_FROM_CLASS (klass),
					   G_SIGNAL_RUN_FIRST |
					   G_SIGNAL_NO_RECURSE,
					   G_STRUCT_OFFSET (MonkeyClass, shooter_up),
					   NULL, NULL,
					   g_cclosure_marshal_VOID__VOID,
					   G_TYPE_NONE,
					   0,NULL);


	signals[SHOOTER_DOWN] = g_signal_new ("down",
					      G_TYPE_FROM_CLASS (klass),
					      G_SIGNAL_RUN_FIRST |
					      G_SIGNAL_NO_RECURSE,
					      G_STRUCT_OFFSET (MonkeyClass, shooter_down),
					      NULL, NULL,
					      g_cclosure_marshal_VOID__VOID,
					      G_TYPE_NONE,
					      0,NULL);




	signals[SHOOTER_CENTER]= g_signal_new ("center",
					       G_TYPE_FROM_CLASS (klass),
					       G_SIGNAL_RUN_FIRST |
					       G_SIGNAL_NO_RECURSE,
					       G_STRUCT_OFFSET (MonkeyClass, shooter_center),
					       NULL, NULL,
					       g_cclosure_marshal_VOID__VOID,
					       G_TYPE_NONE,
					       0,NULL);

	signals[HURRY_UP]  = g_signal_new ("hurry-up",
					   G_TYPE_FROM_CLASS (klass),
					   G_SIGNAL_RUN_FIRST |
					   G_SIGNAL_NO_RECURSE,
					   G_STRUCT_OFFSET (MonkeyClass, hurry_up),
					   NULL, NULL,
					   g_cclosure_marshal_VOID__VOID,
					   G_TYPE_NONE,
					   0,NULL);

	signals[BUBBLES_EXPLODED]= g_signal_new ("bubbles-exploded",
						 G_TYPE_FROM_CLASS (klass),
						 G_SIGNAL_RUN_FIRST |
						 G_SIGNAL_NO_RECURSE,
						 G_STRUCT_OFFSET (MonkeyClass, bubbles_exploded),
						 NULL, NULL,
						 monkey_marshal_VOID__POINTER_POINTER,
						 G_TYPE_NONE,
						 2, G_TYPE_POINTER,G_TYPE_POINTER);

	signals[BUBBLE_SHOT]= g_signal_new ("bubble-shot",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST |
					    G_SIGNAL_NO_RECURSE,
					    G_STRUCT_OFFSET (MonkeyClass, bubble_shot),
					    NULL, NULL,
					    g_cclosure_marshal_VOID__POINTER,
					    G_TYPE_NONE,
					    1, G_TYPE_POINTER);

	signals[BUBBLE_STICKED]= g_signal_new ("bubble-sticked",
					       G_TYPE_FROM_CLASS (klass),
					       G_SIGNAL_RUN_FIRST |
					       G_SIGNAL_NO_RECURSE,
					       G_STRUCT_OFFSET (MonkeyClass, bubble_sticked),
					       NULL, NULL,
					       g_cclosure_marshal_VOID__POINTER,
					       G_TYPE_NONE,
					       1, G_TYPE_POINTER);


	signals[BUBBLES_WAITING_CHANGED]= g_signal_new ("bubbles-waiting-changed",
							G_TYPE_FROM_CLASS (klass),
							G_SIGNAL_RUN_FIRST |
							G_SIGNAL_NO_RECURSE,
							G_STRUCT_OFFSET (MonkeyClass, bubbles_waiting_changed),
							NULL, NULL,
							g_cclosure_marshal_VOID__INT,
							G_TYPE_NONE,
							1, G_TYPE_INT);
}


GType monkey_get_type(void) {
	static GType monkey_type = 0;
    
	if (!monkey_type) {
		static const GTypeInfo monkey_info = {
			sizeof(MonkeyClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) monkey_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof(Monkey),
			1,              /* n_preallocs */
			(GInstanceInitFunc) monkey_instance_init,
		};


		monkey_type = g_type_register_static(G_TYPE_OBJECT,
						     "Monkey",
						     &monkey_info,
						     0);


      
	}
    
	return monkey_type;
}

