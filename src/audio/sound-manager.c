/* sound-manager.c
 * Copyright (C) 2002 Laurent Belmonte
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
#include <gtk/gtk.h>
#include "sound-manager.h"
#include "playground.h"
#include <gst/gst.h>

#define PRIVATE(sound_manager) (sound_manager->private)

static GObjectClass* parent_class = NULL;

struct SoundManagerPrivate {
  GstElement * main_bin;
  GstElement * filesrc;
  GstElement * oggdemux;
  GstElement * vorbis_dec;
  GstElement * audioconvert;
  GstElement * audioscale;
  GstElement * output;

  gchar ** samples_path;

  gchar * current_music_path;

  gboolean active;
};


static void stop_play(SoundManager * m);
static void start_play(SoundManager *m, gchar * path);
static SoundManager * sound_manager_new( void );

static void sound_manager_instance_init(SoundManager * sound_manager) {
  sound_manager->private =g_new0 (SoundManagerPrivate, 1);			
}

static void sound_manager_finalize(GObject* object) {
  SoundManager * sound_manager = SOUND_MANAGER(object);

  g_free(sound_manager->private);

  if (G_OBJECT_CLASS (parent_class)->finalize) {
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
  }
}


static SoundManager * instance = NULL;
SoundManager * sound_manager_get_instance() {
  if( instance == NULL) {
    instance = sound_manager_new();
  }

  return instance;
}

void sound_manager_init(SoundManager * m,gboolean active) 
{
  PRIVATE(m)->active = active;
}


static void sound_manager_class_init (SoundManagerClass *klass) {
  GObjectClass* object_class;
    
  parent_class = g_type_class_peek_parent(klass);
  object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = sound_manager_finalize;
}


GType sound_manager_get_type(void) {
  static GType sound_manager_type = 0;
    
  if (!sound_manager_type) {
    static const GTypeInfo sound_manager_info = {
      sizeof(SoundManagerClass),
      NULL,           /* base_init */
      NULL,           /* base_finalize */
      (GClassInitFunc) sound_manager_class_init,
      NULL,           /* class_finalize */
      NULL,           /* class_data */
      sizeof(SoundManager),
      1,              /* n_preallocs */
      (GInstanceInitFunc) sound_manager_instance_init,
    };



    sound_manager_type = g_type_register_static(G_TYPE_OBJECT,
						"SoundManager",
						&sound_manager_info,
						0);


      
  }
    
  return sound_manager_type;
}


static gboolean
restart_play(gpointer data)
{
  SoundManager * m;

  m = SOUND_MANAGER(data);
  stop_play(m);

  start_play(m,PRIVATE(m)->current_music_path);
  return FALSE;
}

static gboolean
bus_call(GstBus* bus, GstMessage* msg, gpointer data) {
	switch (GST_MESSAGE_TYPE (msg)) {
	case GST_MESSAGE_EOS:
		g_timeout_add(100, restart_play, data);
		break;
	case GST_MESSAGE_ERROR: {
			gchar *debug;
			GError *err;

			gst_message_parse_error (msg, &err, &debug);
			g_free (debug);

			g_print ("Error: %s\n", err->message);
			g_error_free (err);
		} break;
	default:
		break;
	}

	return TRUE;
}


static SoundManager * sound_manager_new( void ) {
  SoundManager * sound_manager;
  SoundManagerPrivate * dp= NULL;

  sound_manager = SOUND_MANAGER (g_object_new (TYPE_SOUND_MANAGER, NULL));

  dp = PRIVATE(sound_manager);
  dp->main_bin = NULL;
  // cache samples here ..
  
  dp->samples_path = g_new0(gchar *, NO_SAMPLE);
  dp->samples_path[ MB_SAMPLE_REBOUND] = 
    DATADIR"/monkey-bubble/sounds/rebound.wav";
  dp->samples_path[ MB_SAMPLE_SHOOT] =
    DATADIR"/monkey-bubble/sounds/launch.wav";

  dp->samples_path[ MB_SAMPLE_EXPLODE] = 
    DATADIR"/monkey-bubble/sounds/destroy_group.wav";

  dp->samples_path[ MB_SAMPLE_STICK] = 
    DATADIR"/monkey-bubble/sounds/stick.wav";

  return sound_manager;
}


static void
stop_play(SoundManager * m) 
{
	if(!PRIVATE(m)->active) {
		return;
	}

	if(PRIVATE(m)->main_bin) {
		gst_element_set_state(PRIVATE(m)->main_bin, GST_STATE_PAUSED);
		g_object_unref(PRIVATE(m)->main_bin);
		PRIVATE(m)->main_bin = NULL;
	}

	g_object_unref( G_OBJECT(PRIVATE(m)->main_bin ));
	  
	PRIVATE(m)->output = NULL;
	PRIVATE(m)->main_bin = NULL;
	PRIVATE(m)->vorbis_dec = NULL;
	PRIVATE(m)->filesrc = NULL;
}

static void 
oggdemux_new_pad(GstElement *gstelement,
		 GObject *new_pad,
		 SoundManager * m)
{
	GstPad *sinkpad = gst_element_get_pad (PRIVATE(m)->vorbis_dec, "sink");
	gst_pad_link (GST_PAD(new_pad), sinkpad);
	gst_object_unref (sinkpad);
}


static void 
start_play(SoundManager *m, gchar * path) 
{
  g_print("start play %s\n",path);
  if( PRIVATE(m)->active) {
    PRIVATE(m)->main_bin = gst_pipeline_new("bin");
    PRIVATE(m)->filesrc = gst_element_factory_make("filesrc","filesrc");
    PRIVATE(m)->oggdemux = gst_element_factory_make("oggdemux","oggdemux");
    PRIVATE(m)->vorbis_dec = gst_element_factory_make("vorbisdec","vorbisdec");
    PRIVATE(m)->audioconvert = gst_element_factory_make("audioconvert","audioconvert");
    PRIVATE(m)->audioscale = gst_element_factory_make("audioscale", "audioscale");
    PRIVATE(m)->output =  gst_element_factory_make("alsasink", "alsa-output");

    PRIVATE(m)->current_music_path = g_strdup(path);

    g_object_set( G_OBJECT( PRIVATE(m)->filesrc), "location",path,NULL);

    // g_object_set( G_OBJECT( PRIVATE(m)->filesrc), "blocksize",10000,NULL);
    gst_bus_add_watch (gst_pipeline_get_bus (
			GST_PIPELINE (PRIVATE(m)->main_bin)),
		    	bus_call, m);
    gst_bin_add_many(GST_BIN(PRIVATE(m)->main_bin),
		     PRIVATE(m)->filesrc,
		     PRIVATE(m)->oggdemux,
		     PRIVATE(m)->vorbis_dec,
		     PRIVATE(m)->audioconvert,
		     PRIVATE(m)->output,
		     NULL);
    gst_element_link(PRIVATE(m)->filesrc, PRIVATE(m)->oggdemux);
    gst_element_link_many(PRIVATE(m)->vorbis_dec,
		          PRIVATE(m)->audioconvert,
			  PRIVATE(m)->output,
			  NULL);
    g_signal_connect(PRIVATE(m)->oggdemux, "pad-added",
		     G_CALLBACK(oggdemux_new_pad), m);

    gst_element_set_state( PRIVATE(m)->main_bin,GST_STATE_PLAYING);
  }
}

void sound_manager_play_music_file(SoundManager *m, gchar * path) {

  g_assert(IS_SOUND_MANAGER(m));

  if(PRIVATE(m)->main_bin) {
    stop_play(m);		
  }
  
  start_play(m,path);
}


void sound_manager_play_fx(SoundManager *m,MbSample sample) 
{
  g_print("play sample %d \n",sample);
}
