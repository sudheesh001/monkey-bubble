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
#include <gst/gconf/gconf.h>
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

	 gboolean new_pad_ok;
  gboolean is_playing;
  gint idle_id;

	 gchar * current_music_path;
};


static void stop_play(SoundManager * m);
static void start_play(SoundManager *m, gchar * path);

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


static gboolean sound_active = TRUE;

void sound_manager_active_sound(gboolean active) {
  sound_active = active;
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

static void eos(GstElement *e ,gpointer user_data) {
    g_print("end of file..\n");

 g_idle_add(restart_play,user_data);
}
static void error_handler(GstElement * e,GObject * o,gchar * string,gpointer data) {
    g_print("error : %s\n",string);

}


SoundManager * sound_manager_new( void ) {
    SoundManager * sound_manager;
    SoundManagerPrivate * dp= NULL;

    sound_manager = SOUND_MANAGER (g_object_new (TYPE_SOUND_MANAGER, NULL));

    dp = PRIVATE(sound_manager);
  
    dp->is_playing = FALSE;			       
    return sound_manager;
}


static void
stop_play(SoundManager * m) 
{
    if( sound_active) {

		  if( PRIVATE(m)->is_playing ) {
				
				PRIVATE(m)->is_playing = FALSE;

				gst_element_set_state( PRIVATE(m)->main_bin,GST_STATE_NULL);
		  }
		  

		  
		  g_object_unref( G_OBJECT(PRIVATE(m)->main_bin ));
		  
		  PRIVATE(m)->output = NULL;
		  PRIVATE(m)->main_bin = NULL;
		  PRIVATE(m)->vorbis_dec = NULL;
		  PRIVATE(m)->filesrc = NULL;
	 }
}

static void 
oggdemux_new_pad(GstElement *gstelement,
		 GObject *new_pad,
		 SoundManager * m)
{
    gst_element_set_state (PRIVATE(m)->main_bin, GST_STATE_PAUSED);

    gst_bin_add_many(GST_BIN(PRIVATE(m)->main_bin),
		     PRIVATE(m)->vorbis_dec,
		     PRIVATE(m)->audioconvert,
		     PRIVATE(m)->audioscale,
		     PRIVATE(m)->output,
		     NULL);

  
  gst_element_link_many(
		     PRIVATE(m)->vorbis_dec,
		     PRIVATE(m)->audioconvert,
		     PRIVATE(m)->audioscale,
		     PRIVATE(m)->output,
			NULL);
  
  GstPad * pad = gst_element_get_pad( GST_ELEMENT(PRIVATE(m)->vorbis_dec),"sink");

  gst_pad_link( GST_PAD(new_pad), pad);
  PRIVATE(m)->new_pad_ok = TRUE;

    
  gst_element_set_state (GST_ELEMENT(PRIVATE(m)->main_bin), GST_STATE_PLAYING); 
}


static void 
start_play(SoundManager *m, gchar * path) 
{
  if( sound_active) {
		PRIVATE(m)->new_pad_ok = FALSE;
		PRIVATE(m)->main_bin = gst_thread_new("bin");
		PRIVATE(m)->filesrc = gst_element_factory_make("filesrc","filesrc");
		PRIVATE(m)->oggdemux = gst_element_factory_make("oggdemux","oggdemux");
		PRIVATE(m)->vorbis_dec = gst_element_factory_make("vorbisdec","vorbisdec");
		PRIVATE(m)->audioconvert = gst_element_factory_make("audioconvert","audioconvert");
		PRIVATE(m)->audioscale = gst_element_factory_make("audioscale", "audioscale");
		PRIVATE(m)->output =  gst_gconf_get_default_audio_sink();

		PRIVATE(m)->current_music_path = g_strdup(path);

		g_signal_connect(G_OBJECT(PRIVATE(m)->main_bin),
							  "error", GTK_SIGNAL_FUNC(error_handler),NULL);
    
		g_signal_connect( PRIVATE(m)->oggdemux, "new-pad", G_CALLBACK(oggdemux_new_pad),m);
		
		gst_bin_add_many( GST_BIN( PRIVATE(m)->main_bin),
								PRIVATE(m)->filesrc,
								PRIVATE(m)->oggdemux,
								NULL);
		
		g_object_set( G_OBJECT( PRIVATE(m)->filesrc), "location",path,NULL);
		
		g_object_set( G_OBJECT( PRIVATE(m)->filesrc), "blocksize",10000,NULL);
		
		g_signal_connect(PRIVATE(m)->main_bin,
							  "eos",
							  G_CALLBACK(eos),
							  m);
	 
		gst_element_link_many( PRIVATE(m)->filesrc, PRIVATE(m)->oggdemux,NULL);
		
		
		gst_element_set_state( PRIVATE(m)->main_bin,GST_STATE_PLAYING);
		
		PRIVATE(m)->is_playing = TRUE;
		
  }
}

void sound_manager_play_music_file(SoundManager *m, gchar * path) {

  g_assert(IS_SOUND_MANAGER(m));

  if( PRIVATE(m)->is_playing == TRUE) {
		stop_play(m);		
  }
  
  start_play(m,path);
}
