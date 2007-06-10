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

static void _pad_removed     (GstElement *gstelement,
			      GObject    *old_pad,
			      gpointer    user_data)       
{
  //  g_print("pad removed !\n");
}

/* we need to define these two functions */
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
  priv->pipeline = gst_pipeline_new("bin");

  fakesrc = gst_element_factory_make("audiotestsrc","testaudifakesrc");
  g_object_set( G_OBJECT( fakesrc ),
		"wave",4,"volume",0.6,NULL);
  
  priv->adder = gst_element_factory_make("adder","adder");
  output =  gst_element_factory_make("autoaudiosink", "audio-output");  

  gst_bus_add_watch (gst_pipeline_get_bus (GST_PIPELINE (priv->pipeline)),
		     _bus_message_received, self);
  

  gst_bin_add_many(GST_BIN(priv->pipeline),
		   fakesrc,
		   output,
		   priv->adder,
		   NULL);

  gst_element_link_many(priv->adder,
			output,
			NULL);


  g_signal_connect( priv->adder, "pad-removed",G_CALLBACK( _pad_removed), NULL );
  
  pad = gst_element_get_request_pad( priv->adder, "sink%d");
  
  gst_pad_link(gst_element_get_pad( fakesrc, "src"), pad);
 
  // Launch the pipeline
  gst_element_set_state( priv->pipeline,GST_STATE_PLAYING);
}




MbAudioEngine * mb_audio_engine_new(){
  MbAudioEngine * self;

  self = MB_AUDIO_ENGINE( g_object_new(MB_TYPE_AUDIO_ENGINE,NULL));

  mb_audio_engine_init( self);

  return self;
  
}


struct ConnectStruct {
  GstElement * adder;
  GstPad * pad;
  GstElement * bin;
};

struct EosStruct {
  MbAudioEngine * engine;
  gint id;
  GstElement * element;
};

void 
_audiobin_new_pad(gpointer data,
		  GstPad     *pad,
		  gboolean    last)
{

  GstCaps *caps;
  GstStructure *str;
  GstPad *audiopad;
  
  struct ConnectStruct * t = (struct ConnectStruct *)data;
  audiopad = t->pad;//gst_element_get_pad (priv->audioconvert, "sink");
  
  if (GST_PAD_IS_LINKED (audiopad)) {
    g_object_unref (audiopad);
    return;
  }
  
  caps = gst_pad_get_caps (pad);
  str = gst_caps_get_structure (caps, 0);
  
  if (!g_strrstr (gst_structure_get_name (str), "audio")) {
    gst_caps_unref (caps);
    gst_object_unref (audiopad);
    return;
  }
  
  gst_caps_unref (caps);
  
  gst_pad_link (pad, audiopad);
 
  gst_element_link(t->bin,t->adder);//gst_element_get_pad(e,"src")  


  gst_element_get_parent(gst_pad_get_parent( audiopad));

}

void 
mb_audio_engine_cache_audio_file(MbAudioEngine * self,
				 const gchar * path)
{

  Private * priv;
  GstMbmemContent * content = NULL;


  priv = GET_PRIVATE(self);
  content = GST_MBMEM_CONTENT( g_object_new( GST_TYPE_MBMEM_CONTENT, NULL ) );
  
  gst_mbmem_content_load_file(content,
                              path,
                              NULL);

  g_hash_table_insert(priv->cached_files,g_strdup(path),content);
}

static int id = 0;


GstElement * _create_audiobin(MbAudioEngine * self,const gchar * path)
{
  GstElement * filesrc;
  GstElement * decodebin;
  GstElement * audioconvert;
  GstElement * bin;
  GstPad * pad;

  int t = id;
  id++;
  gchar * name = (gchar *)  g_new0( gchar *,255);
  
  g_sprintf(name,"filesrc%d",t);
  filesrc = gst_element_factory_make("filesrc",name);
  g_object_set(filesrc, "location",path,NULL);

  g_sprintf(name,"decodebin%d",t);  
  decodebin = gst_element_factory_make ("decodebin", name);



  g_sprintf(name,"musicbin%d",t);  
  bin = gst_bin_new(name);
  
  g_sprintf(name,"audioconvert%d",t);    
  audioconvert = gst_element_factory_make("audioconvert",name);
  
  

  gst_bin_add_many( GST_BIN(bin),
                    filesrc,decodebin,audioconvert,NULL);


  gst_element_link(filesrc, decodebin);

  pad =                
    gst_ghost_pad_new ("src", 
		       gst_element_get_pad(audioconvert,"src"));

  gst_element_add_pad ( GST_ELEMENT(bin),pad);

  if( pad == NULL ) g_print(" pad is null \n");
  g_object_ref(pad);

  struct ConnectStruct * datas;
  datas = g_new0(struct ConnectStruct,1);
  
  datas->adder = GET_PRIVATE(self)->adder;
  datas->bin = bin;
  datas->pad = gst_element_get_pad(audioconvert,"sink");

  g_signal_connect_swapped(decodebin, "new-decoded-pad", 
			   G_CALLBACK (_audiobin_new_pad), datas );
  g_free(name);

  return bin;
}


gboolean _stop_element( struct EosStruct * datas)
{
Private * priv;
  MbAudioEngine * self = datas->engine;
  priv = GET_PRIVATE(self);
  gst_element_set_state( datas->element,GST_STATE_NULL);
  gst_element_set_locked_state        (datas->element,
				       TRUE);
  
  //  gst_element_set_state( priv->pipeline,GST_STATE_PAUSED);
  //  gst_bin_remove( priv->pipeline, datas->element );
  //  gst_element_set_state( priv->pipeline,GST_STATE_PLAYING);
    //    g_free(datas);

      return FALSE;
}


gboolean _remove_element( MbAudioEngine * self, GstElement * e)
{
  
  Private * priv;
  priv = GET_PRIVATE(self);
	
  //  gst_element_set_state( priv->pipeline,GST_STATE_PAUSED);
    GstPad * p = gst_pad_get_peer(gst_element_get_pad(e,"src"));
  gst_element_unlink( e, priv->adder);
   gst_element_release_request_pad(priv->adder,p);
      gst_bin_remove( priv->pipeline, e );
  
 priv->samples = g_list_remove( priv->samples, e);
 // gst_element_set_state( priv->pipeline,GST_STATE_PLAYING);

    //    g_free(datas);

      return FALSE;
}

static gint file_id = 1;

static gboolean _event_handler(GstPad * pad,GstEvent * event, 
			       struct EosStruct * datas)
{

  Private * priv;
  MbAudioEngine * self = datas->engine;
  priv = GET_PRIVATE(self);
  if( GST_EVENT_TYPE(event ) == GST_EVENT_EOS) {

    //   g_hash_table_remove( priv->elements_map,( gpointer) datas->id );
    //    gst_element_set_state( datas->element,GST_STATE_NULL);
        g_idle_add( _stop_element,datas );
    //        gst_element_set_state( priv->pipeline,GST_STATE_PAUSED);
    //        
	
    //        gst_bin_remove( priv->pipeline, datas->element );
    //        gst_element_set_state( priv->pipeline,GST_STATE_PLAYING);
	//        g_free(datas);
    return TRUE;
  } else {
    //    g_printf("event %s\n", GST_EVENT_TYPE_NAME(event));
    return TRUE;
  }
}

gint
mb_audio_engine_play_audio_file(MbAudioEngine * self,
				const gchar * path)
{
  
  Private * priv;
  GstMbmemContent * content = NULL;
  GstElement * e;
  gint current_id;
  priv = GET_PRIVATE(self);

  content = GST_MBMEM_CONTENT(g_hash_table_lookup(priv->cached_files,path));
  // gst_element_set_state( priv->pipeline,GST_STATE_PAUSED);

  if( content == NULL ) {
    // play directly
    e = _create_audiobin(self, path );
    gst_bin_add( GST_BIN( priv->pipeline), e);
  } else {
  
    // cached sample
     e = GST_ELEMENT( g_object_new( GST_TYPE_MBMEM_SRC, NULL ) );
    gst_mbmem_src_set_content(GST_MBMEM_SRC(e),content);
    gst_bin_add( GST_BIN( priv->pipeline), e);
    gst_element_link(e,priv->adder);  
    priv->samples = g_list_append( priv->samples, e);
  }
  

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
    gst_element_set_state(e,GST_STATE_NULL);
    gst_element_set_locked_state (e, TRUE);
    gst_bin_remove( GST_BIN( priv->pipeline ), e);
    g_hash_table_remove( priv->elements_map, (gpointer)file_id );
  } else {
    g_print("id not found \n");
  }

}


static gboolean _bus_message_received (GstBus *bus,
				       GstMessage *message,
				       gpointer data)
{

  
  Private * priv;
  
  MbAudioEngine * self;
  self = MB_AUDIO_ENGINE(data);
  
  priv = GET_PRIVATE(self);


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
    case GST_MESSAGE_EOS:
      g_print("eos .. \n");
      /* end-of-stream */
      break;
  case GST_MESSAGE_STATE_CHANGED: {
    GstElement * e;
    GstState state,pending;
    e = GST_ELEMENT(GST_MESSAGE_SRC(message));
    //    g_print("state changed for %s \n",gst_object_get_name(e));
    gst_element_get_state(e,&state,&pending,GST_CLOCK_TIME_NONE );
    if( state == GST_STATE_NULL && g_list_find( priv->samples, e ) != NULL ) {
            _remove_element(self,e);
	    //    g_print("state %d, pending %d \n",state,pending);

    }
  }
  default:
    //    g_print("message %s\n",GST_MESSAGE_TYPE_NAME(message));
    /* unhandled message */
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
