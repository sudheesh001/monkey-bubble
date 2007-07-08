/* mb-music.c
 * Copyright (C) 2006 Laurent Belmonte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include <glib/gprintf.h>

#include <gst/gst.h>

#include "mb-music-bin.h"
#include "gst-mbmem-src.h"

/* ******************* *
*  GType Declaration  *
* ******************* */

typedef struct _Private {
	gchar * path;
	gchar * name;
	GstElement * musicconvert;
	GstElement * bin;
} Private;


G_DEFINE_TYPE(MbMusicBin,mb_music_bin,G_TYPE_OBJECT);

#define GET_PRIVATE(o)  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_TYPE_MUSIC_BIN, Private))


static GObjectClass* parent_class = NULL;

static void _finalize(MbMusicBin * self);

static void _create_bin(MbMusicBin * self);
static void _musicbin_new_pad(MbMusicBin * self,GstPad     *pad,gboolean    last);

MbMusicBin * mb_music_bin_new(const gchar * bin_name,const gchar * path)
{

	Private * priv;
	MbMusicBin * self = NULL;
	self = MB_MUSIC_BIN( g_object_new(MB_TYPE_MUSIC_BIN, NULL));

	priv = GET_PRIVATE(self);
	priv->name = g_strdup(bin_name);
	priv->path = g_strdup(path);
	_create_bin(self);

	return self;
}


static void mb_music_bin_init (MbMusicBin *self) 
{
	Private * priv;
	priv = GET_PRIVATE(self);
	priv->path = NULL;
	priv->name = NULL;
	priv->musicconvert = NULL;
	priv->bin = NULL;
}

void _create_bin(MbMusicBin * self)
{
	GstElement * filesrc;
	GstElement * decodebin;
	GstElement * musicconvert;
 	GstElement * bin;
	GstPad * pad;

	Private * priv;
  
	priv = GET_PRIVATE(self);

	gchar * name = (gchar *)  g_new0( gchar *,255);
  
	g_sprintf(name,"filesrc%s",priv->name);
	filesrc = gst_element_factory_make("filesrc",name);
  	g_object_set(filesrc, "location",priv->path,NULL);

	g_print("path %s \n",priv->path);
	g_sprintf(name,"decodebin%s",priv->name);  
	decodebin = gst_element_factory_make ("decodebin", name);
	
	g_sprintf(name,"musicbin%s",priv->name);  
	bin = gst_bin_new(name);
  
	g_sprintf(name,"audioconvert%s",priv->name);    
	musicconvert = gst_element_factory_make("audioconvert",name);
	priv->musicconvert = musicconvert;
	gst_bin_add_many( GST_BIN(bin),filesrc,decodebin,musicconvert,NULL);

	gst_element_link(filesrc,decodebin);

	pad = gst_ghost_pad_new ("src",gst_element_get_pad(musicconvert,"src"));
	gst_element_add_pad ( GST_ELEMENT(bin),pad);

	if( pad == NULL ) g_print(" pad is null \n");
  
	g_object_ref(pad);

  	g_signal_connect_swapped(decodebin, "new-decoded-pad", G_CALLBACK (_musicbin_new_pad), self );
	priv->bin = bin;
	g_free(name); 
}

GstElement * mb_music_bin_get_element(MbMusicBin * self)
{
	return GET_PRIVATE(self)->bin;
}

static void _musicbin_new_pad(MbMusicBin * self,GstPad     *pad,gboolean    last)
{

	GstCaps *caps;
	GstStructure *str;
	GstPad *musicpad;

	Private * priv;
	priv = GET_PRIVATE(self);

	musicpad = gst_element_get_pad(priv->musicconvert, "sink");
	if (GST_PAD_IS_LINKED (musicpad)) {
		g_object_unref (musicpad);
		return;
	}
  
	caps = gst_pad_get_caps (pad);
	str = gst_caps_get_structure (caps, 0);
  
	if (!g_strrstr (gst_structure_get_name (str), "audio")) {
		gst_caps_unref (caps);
		gst_object_unref (musicpad);
		return;
	}
  
	gst_caps_unref (caps);
  	gst_pad_link (pad, musicpad);
}

static void _finalize(MbMusicBin * self) 
{
	Private * priv;
	priv = GET_PRIVATE(self);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT(self));
	}  
}

static void mb_music_bin_class_init (MbMusicBinClass *klass) 
{
	parent_class = g_type_class_peek_parent(klass);

	g_type_class_add_private (klass, sizeof (Private));
  
	G_OBJECT_CLASS (klass)->finalize = (GObjectFinalizeFunc) _finalize;

}
