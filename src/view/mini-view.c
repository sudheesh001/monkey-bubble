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


#include <string.h>
#include <stdio.h>

#include <glib-object.h>
#include <glib.h>

#include "mini-view.h"

#define ROW_COUNT 13
#define COLUMN_COUNT 8

#define PRIVATE(self) (self->private )
static GObjectClass* parent_class = NULL;

typedef struct ColorBlock {
	Color c;
	Block * b;
} ColorBlock;

struct MbMiniViewPrivate 
{
	MonkeyCanvas * canvas;
	Color * colors;
	ColorBlock ** blocks;
	Layer * layer;
	gint last_odd;
};


MbMiniView *
mb_mini_view_new  (MonkeyCanvas * canvas,
		   gint x,gint y)
{
	MbMiniView * self;

	self = MB_MINI_VIEW( g_object_new(MB_TYPE_MINI_VIEW,NULL) );

	PRIVATE(self)->canvas = canvas;
	g_object_ref( canvas);

	PRIVATE(self)->layer = monkey_canvas_append_layer(canvas,x,y);

	PRIVATE(self)->blocks =
		g_malloc( sizeof(ColorBlock *) * ROW_COUNT * COLUMN_COUNT);

	memset( PRIVATE(self)->blocks, 0,
		sizeof(ColorBlock *) * ROW_COUNT * COLUMN_COUNT);

	PRIVATE(self)->colors = 
		g_malloc( sizeof(Color) * ROW_COUNT * COLUMN_COUNT);

	memset( PRIVATE(self)->colors,
		NO_COLOR, sizeof(Color) * ROW_COUNT * COLUMN_COUNT);

	PRIVATE(self)->last_odd = 10;
	return self;
}

static void 
mb_mini_view_instance_init(MbMiniView * self) 
{
	PRIVATE(self) = g_new0 (MbMiniViewPrivate, 1);			
}


static void
mb_mini_view_finalize (GObject * object) 
{
	MbMiniView * self;
	int i;
	self = MB_MINI_VIEW(object);

	for(i = 0 ; i < ROW_COUNT * COLUMN_COUNT; i++) {
		if( PRIVATE(self)->blocks[i] != NULL) {
			monkey_canvas_remove_block(PRIVATE(self)->canvas,
						   PRIVATE(self)->blocks[i]->b);

			monkey_canvas_unref_block( PRIVATE(self)->canvas,
						   PRIVATE(self)->blocks[i]->b);
			
			PRIVATE(self)->blocks[i] = NULL;
		}

	}

	g_free( PRIVATE(self)->blocks);
	PRIVATE(self)->blocks = NULL;

	g_free( PRIVATE(self)->colors);
	PRIVATE(self)->colors = NULL;
	
	// free layer ?

	g_object_unref( PRIVATE(self)->canvas);
	PRIVATE(self)->canvas = NULL;
	
	g_free(PRIVATE(self));
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}

}


static ColorBlock * create_bubble(MbMiniView * self,
				  Color color ) {
	
	
        Block * block;
	ColorBlock * cb;
	
        gchar path[4096];
        gint str_length;


        strcpy( path ,
                DATADIR"/monkey-bubble/gfx/bubbles/little_bubble_");
	
        str_length = strlen(path);

    
        sprintf(path+str_length ,"0%d.svg",color+1);
    

        
	
        block = monkey_canvas_create_block_from_image(PRIVATE(self)->canvas,
						      path,
						      16,16,
						      8,8);

	cb = g_malloc( sizeof(ColorBlock));
	cb->b = block;
	cb->c = color;
        return cb;
}


static void
remove_colorblock(MbMiniView * self,int i)
{

	if( PRIVATE(self)->blocks[i] != NULL) {
			
		monkey_canvas_remove_block(PRIVATE(self)->canvas,
					   PRIVATE(self)->blocks[i]->b);
			
		monkey_canvas_unref_block( PRIVATE(self)->canvas,
					   PRIVATE(self)->blocks[i]->b);
	
		g_free( PRIVATE(self)->blocks[i]);
		PRIVATE(self)->blocks[i] = NULL;
		
	}

}

static void
add_colorblock(MbMiniView * self,int i,int x,int y,Color c,int odd)
{
	ColorBlock * b;
	gboolean add_block = TRUE;
	if( PRIVATE(self)->blocks[i] != NULL) {
		
		if(  PRIVATE(self)->blocks[i]->c != c) {
			remove_colorblock(self,i);
		} else {
			add_block = FALSE;
		}
		
		
	} 


	if( add_block == TRUE) {
			
		b = create_bubble(self,c);
		if( odd == 0) {
			monkey_canvas_add_block( PRIVATE(self)->canvas,
						 PRIVATE(self)->layer,
						 b->b, x*16 + 8 ,y*14);

		} else {

			monkey_canvas_add_block( PRIVATE(self)->canvas,
						 PRIVATE(self)->layer,
						 b->b, x*16   ,y*14);
		}
		PRIVATE(self)->blocks[i] = b;

	}

}

void 
mb_mini_view_update(MbMiniView * self,
		    Color * bubbles,gint odd)
{

	int i,x,y;
	x = 0;
	y = 0;
	for(i = 0 ; i < ROW_COUNT * COLUMN_COUNT; i++) {
	
				if( bubbles[i] != NO_COLOR) {
					
					if( odd != PRIVATE(self)->last_odd) {
						remove_colorblock(self,i);
					}
					add_colorblock(self,i,x,y,bubbles[i],(odd+y ) %2);
				} else {
					remove_colorblock(self,i);
				}

		x++;
		if( x >= COLUMN_COUNT) {
			x = 0;
			y++;
		}

	}

	PRIVATE(self)->last_odd = odd;
}



static void
mb_mini_view_class_init (MbMiniViewClass	* mb_mini_view_class)
{
	GObjectClass	* g_object_class;

	parent_class = g_type_class_peek_parent(mb_mini_view_class);

	g_object_class = G_OBJECT_CLASS(mb_mini_view_class);
	g_object_class->finalize = mb_mini_view_finalize;

}


GType
mb_mini_view_get_type (void)
{
	static GType	type = 0;

	if (!type)
	{
		const GTypeInfo info = {
			sizeof (MbMiniViewClass),
			NULL,	/* base initializer */
			NULL,	/* base finalizer */
			(GClassInitFunc)mb_mini_view_class_init,
			NULL,	/* class finalizer */
			NULL,	/* class data */
			sizeof (MbMiniView),
			1,
			(GInstanceInitFunc) mb_mini_view_instance_init,
			0
		};

		type = g_type_register_static (
				G_TYPE_OBJECT,
				"MbMiniView",
				&info,
				0);
	}

	return type;
}

