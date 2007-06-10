/* mb-mbmem.c
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

#include "gst-mbmem-content.h"

/* ******************* *
*  GType Declaration  *
* ******************* */

G_DEFINE_TYPE(GstMbmemContent,gst_mbmem_content,G_TYPE_OBJECT);


typedef struct _TransientState {
  GstElement * audio;
  GMainLoop * loop;
  GstMbmemContent * self;
} TransientState ;

static GObjectClass* parent_class = NULL;

static void _finalize(GstMbmemContent * self);


static gboolean _bus_message_received (GstBus *bus,
				   GstMessage *message,
				   gpointer data);

/* we need to define these two functions */
static void gst_mbmem_content_init (GstMbmemContent *self) 
{
}



static void
cb_newpad (GstElement *decodebin,
	   GstPad     *pad,
	   gboolean    last,
	   gpointer    data)
{
  GstCaps *caps;
  GstStructure *str;
  GstPad *audiopad;

  TransientState * ts;
  GstElement * audio;
  GstMbmemContent * self;

  ts = (TransientState *) data;
  self = ts->self;
  audio = ts->audio;
  /* only link once */
  audiopad = gst_element_get_pad (audio, "sink");
  if (GST_PAD_IS_LINKED (audiopad)) {
    g_object_unref (audiopad);
    return;
  }

  /* check media type */

  caps = gst_pad_get_caps ( pad);
  str = gst_caps_get_structure (caps, 0);

  if (!g_strrstr (gst_structure_get_name (str), "audio")) {
    gst_caps_unref (caps);
    gst_object_unref (audiopad);
    return;
  }

  gst_caps_unref (caps);
  
  /* link'n'play */
  gst_pad_link (pad, audiopad);


}


static gboolean _bus_message_received (GstBus *bus,
				       GstMessage *message,
				       gpointer data)
{


  TransientState * ts = (TransientState *)data;
  
  switch (GST_MESSAGE_TYPE (message)) {
  case GST_MESSAGE_ERROR: {
    GError *err;
    gchar *debug;
    
    gst_message_parse_error (message, &err, &debug);
    g_print ("Error caching file : %s\n", err->message);
    g_error_free (err);
    g_free (debug);
      
    break;
  }
    case GST_MESSAGE_EOS:
      g_main_loop_quit ( ts->loop);
      /* end-of-stream */
      break;
  case GST_MESSAGE_NEW_CLOCK:
    {
    GstCaps * caps = gst_pad_get_caps (gst_pad_get_peer(gst_element_get_pad (ts->audio,"sink")));

    ts->self->caps = gst_caps_copy( caps);
    gst_caps_unref (caps);
    
    }
    break;
  default:
    /* unhandled message */
    break;
  }
  
  /* remove message from the queue */
  return TRUE;
}

GstCaps * 
gst_mbmem_content_get_caps(GstMbmemContent * self)
{
  
  return gst_caps_copy(self->caps);
}

GstBuffer * gst_mbmem_content_get_buffer(GstMbmemContent * content, int buff )
{
  return g_slist_nth_data( content->buffers, buff);
}


static void _received_new_buffer(gpointer data,
				 GstBuffer   *buffer,
				 GstPad      *pad,
				 gpointer user_data)  
{

  GstMbmemContent * self;



  self = GST_MBMEM_CONTENT(user_data);
  self->buffers = g_slist_append( self->buffers, gst_buffer_copy(buffer) );

}

void 
gst_mbmem_content_load_file(GstMbmemContent * self,
			    const gchar * path,GError ** error)
{

  GstElement * filesrc;
  GstElement * dec;
  GstElement * output;
  GstElement * pipeline;
  GstElement * audio;

  TransientState * ts;
  
  pipeline = gst_pipeline_new ("pipeline");

  
  audio = gst_bin_new ("audiobin");
  
  output =  gst_element_factory_make("fakesink", "alsa-output");
  GstPad * audiopad = gst_element_get_pad (output, "sink");
  
  g_object_set( G_OBJECT( output) ,"signal-handoffs" , TRUE, NULL);  
 
  g_signal_connect( G_OBJECT( output), "handoff", G_CALLBACK(_received_new_buffer), self);
 

  gst_bin_add_many (GST_BIN (audio), output, NULL);


  filesrc = gst_element_factory_make("filesrc","filesrc");
  g_object_set( G_OBJECT(filesrc), "location",path,NULL);
  
  dec = gst_element_factory_make ("decodebin", "decoder");
  
  
  ts = g_new0( TransientState, 1);

  ts->self = self;
  ts->audio = audio;

  g_signal_connect (dec, "new-decoded-pad", G_CALLBACK (cb_newpad), ts);


  gst_bin_add_many (GST_BIN (pipeline), filesrc, dec, NULL);

  gst_element_link (filesrc, dec);






  gst_element_add_pad (audio,
      gst_ghost_pad_new ("sink", audiopad));

  gst_object_unref (audiopad);

  gst_bin_add (GST_BIN (pipeline), audio);



  GMainLoop * loop = g_main_loop_new (NULL, FALSE);
  ts->loop = loop;
  GstBus * bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus,_bus_message_received,ts);
  gst_object_unref (bus);
  
  
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);
  
  g_free(ts);

  gst_element_set_state(pipeline,GST_STATE_NULL);
  g_object_unref(pipeline);

}


static void _finalize(GstMbmemContent * self) 
{

  if (G_OBJECT_CLASS (parent_class)->finalize) {
    (* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT(self));
  }

  
}

static void gst_mbmem_content_class_init (GstMbmemContentClass *klass) 
{
  parent_class = g_type_class_peek_parent(klass);
  
  G_OBJECT_CLASS (klass)->finalize = (GObjectFinalizeFunc) _finalize;

}
