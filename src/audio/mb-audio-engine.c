/* mb-audio.c
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

#include "mb-audio-engine.h"
#include "gst-mbmem-src.h"
#include "mb-music-bin.h"

/* ******************* *
*  GType Declaration  *
* ******************* */

typedef struct _Private {
  GstElement * pipeline;
  GstElement * adder;
  GHashTable * cached_files;
  GHashTable * elements_map;
  GList * samples;
} Private;


G_DEFINE_TYPE(MbAudioEngine,mb_audio_engine,G_TYPE_OBJECT);

#define GET_PRIVATE(o)  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_TYPE_AUDIO_ENGINE, Private))


static GObjectClass* parent_class = NULL;

static void _finalize(MbAudioEngine * self);


static gboolean _bus_message_received (GstBus *bus,
				       GstMessage *message,
				       gpointer data);



MbAudioEngine * mb_audio_engine_new()
{
	MbAudioEngine * self;
	self = MB_AUDIO_ENGINE( g_object_new(MB_TYPE_AUDIO_ENGINE,NULL));
	mb_audio_engine_init( self);
	return self;  
}

static void _autoaudio_new_element(GstBin     *bin,
			     GstElement *element,
				   gpointer data)
{
  // hope this work for every audiosink ..
  g_object_set (element,
		"buffer-time",(gint64)20000,NULL);

}

static void mb_audio_engine_init (MbAudioEngine *self) 
{

	Private * priv;
	GstElement * fakesrc;
	GstElement * output;
	GstPad * pad;


	priv = GET_PRIVATE(self);
	priv->elements_map = g_hash_table_new(g_direct_hash,g_direct_equal);
	
	priv->cached_files = g_hash_table_new(g_str_hash,g_str_equal);

	priv->samples = NULL;

	// create main pipeline
	priv->pipeline = gst_pipeline_new("bin");
	// use fakesrc to make sure pipeline is never stopped
	fakesrc = gst_element_factory_make("audiotestsrc","testaudifakesrc");

	g_object_set( G_OBJECT( fakesrc ),"wave",4,"volume",0.6,NULL);
  	priv->adder = gst_element_factory_make("adder","adder");
	// use auto audiosink
	output =  gst_element_factory_make("autoaudiosink", "audio-output");  
	g_signal_connect(output, "element-added", G_CALLBACK (_autoaudio_new_element), self );


	gst_bus_add_watch (gst_pipeline_get_bus (GST_PIPELINE (priv->pipeline)),
			   _bus_message_received, self);
  

  	gst_bin_add_many(GST_BIN(priv->pipeline),fakesrc,output,priv->adder,NULL);
	gst_element_link_many(priv->adder,output,NULL);

	// link fake src  
	pad = gst_element_get_request_pad( priv->adder, "sink%d");
	gst_pad_link(gst_element_get_pad( fakesrc, "src"), pad);
 
	// Launch the pipeline
	gst_element_set_state( priv->pipeline,GST_STATE_PLAYING);
}







struct EosStruct {
  MbAudioEngine * engine;
  gint id;
  GstElement * element;
};


void
mb_audio_engine_cache_audio_file(MbAudioEngine * self,
				 const gchar * path)
{
	Private * priv;
	GstMbmemContent * content = NULL;

	priv = GET_PRIVATE(self);
	content = GST_MBMEM_CONTENT( g_object_new( GST_TYPE_MBMEM_CONTENT, NULL ) );
  	gst_mbmem_content_load_file(content,path,NULL);
	g_hash_table_insert(priv->cached_files,g_strdup(path),content);
}

gboolean _remove_element( MbAudioEngine * self, GstElement * e)
{  
	GstPad * pad;
	Private * priv;

	priv = GET_PRIVATE(self);
	gst_element_set_state( e,GST_STATE_NULL);
	gst_element_set_locked_state(e,TRUE);	
	pad = gst_pad_get_peer(gst_element_get_pad(e,"src"));
	gst_element_unlink( e, priv->adder);
	gst_element_release_request_pad(priv->adder,pad);
	gst_bin_remove( GST_BIN(priv->pipeline), e );
	return FALSE;
}

gboolean _stop_element( struct EosStruct * datas)
{
	Private * priv;
	MbAudioEngine * self = datas->engine;
	priv = GET_PRIVATE(self);
	_remove_element(self,datas->element);
	g_hash_table_remove( priv->elements_map, (gpointer)datas->id );

	return FALSE;
}


static gint file_id = 1;

static gboolean _event_handler(GstPad * pad,GstEvent * event, 
			       struct EosStruct * datas)
{

	Private * priv;
	MbAudioEngine * self;

	self = datas->engine;
	priv = GET_PRIVATE(self);
	if( GST_EVENT_TYPE(event ) == GST_EVENT_EOS) {
		g_idle_add( (GSourceFunc)_stop_element,datas );
		return TRUE;
  	} else {
    		return TRUE;
	}
}

gint
mb_audio_engine_play_audio_file(MbAudioEngine * self,
				const gchar * path)
{
  
	Private * priv;
	GstMbmemContent * content;
	GstElement * e;
	gint current_id;

	priv = GET_PRIVATE(self);

	content = GST_MBMEM_CONTENT(g_hash_table_lookup(priv->cached_files,path));
	if( content == NULL ) {
		// play directly
		MbMusicBin * b = mb_music_bin_new("music",path);
		e = mb_music_bin_get_element(b);
	} else {
		// cached sample
		e = GST_ELEMENT( g_object_new( GST_TYPE_MBMEM_SRC, NULL ) );
		gst_mbmem_src_set_content(GST_MBMEM_SRC(e),content);
	}
	
	gst_bin_add( GST_BIN( priv->pipeline), e);
	gst_element_link(e,priv->adder);  
	current_id = file_id++;
	g_hash_table_insert( priv->elements_map, (gpointer)current_id, e);
  
  	struct EosStruct * datas;
	datas = g_new0(struct EosStruct,1);
	datas->engine = self;
	datas->id = current_id;
	datas->element = e;
	gst_pad_add_event_probe(gst_element_get_pad(e,"src"),G_CALLBACK(_event_handler),datas);
	gst_element_set_state( priv->pipeline,GST_STATE_PLAYING);
	return current_id;
}

void
mb_audio_engine_stop(MbAudioEngine * self,
		     gint file_id)
{

	Private * priv;
	GstElement * e;
	priv = GET_PRIVATE(self);

	e = GST_ELEMENT(g_hash_table_lookup( priv->elements_map,(gpointer) file_id));
	if( e != NULL ) {
		_remove_element(self, e);
		g_hash_table_remove( priv->elements_map, (gpointer)file_id );
	} else {
		// nothing to do. Bad file_id or no more valid
	}

}


static gboolean _bus_message_received (GstBus *bus,
				       GstMessage *message,
				       gpointer data)
{

	switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_ERROR: {
			GError *err;
			gchar *debug;
			gst_message_parse_error (message, &err, &debug);
			g_print ("Error: %s\n", err->message);
			g_error_free (err);
			g_free (debug);    
			break;
		}
		default:
			break;
  	}
  	/* remove message from the queue */
	return TRUE;
}


static void _finalize(MbAudioEngine * self) 
{

	Private * priv;
	priv = GET_PRIVATE(self);
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT(self));
	}
}

static void mb_audio_engine_class_init (MbAudioEngineClass *klass) 
{
	parent_class = g_type_class_peek_parent(klass);
	g_type_class_add_private (klass, sizeof (Private));
	G_OBJECT_CLASS (klass)->finalize = (GObjectFinalizeFunc) _finalize;
}
