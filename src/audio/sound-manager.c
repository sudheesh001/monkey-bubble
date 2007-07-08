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
#include "mb-audio-engine.h"

#define PRIVATE(sound_manager) (sound_manager->private)

static GObjectClass* parent_class = NULL;

struct SoundManagerPrivate {
  MbAudioEngine * engine;
  gchar ** samples_path;
  gint current_music_id;
  MbMusic current_music;
  gboolean active;

};


static SoundManager * sound_manager_new( void );
static void load_samples(SoundManager * m);

static void sound_manager_instance_init(SoundManager * sound_manager) {
  sound_manager->private =g_new0 (SoundManagerPrivate, 1);			
  sound_manager->private->engine = mb_audio_engine_new();

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
  load_samples(m);
  PRIVATE(m)->current_music_id = - 1;
  PRIVATE(m)->current_music = NO_MUSIC;
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


static void load_sample(SoundManager * m,int i) {
  mb_audio_engine_cache_audio_file(  PRIVATE(m)->engine,
				     PRIVATE(m)->samples_path[i]);
}

static void load_samples(SoundManager * m) {
  load_sample(m,0);
  load_sample(m,1);
  load_sample(m,2);
  load_sample(m,3);
}

static SoundManager * sound_manager_new( void ) {
  SoundManager * sound_manager;
  SoundManagerPrivate * dp= NULL;

  sound_manager = SOUND_MANAGER (g_object_new (TYPE_SOUND_MANAGER, NULL));

  dp = PRIVATE(sound_manager);
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


void sound_manager_play_music(SoundManager *m,MbMusic music)
{

	g_assert(IS_SOUND_MANAGER(m));
	if( PRIVATE(m)->current_music_id != -1 ) {
		mb_audio_engine_stop( PRIVATE(m)->engine, PRIVATE(m)->current_music_id );
		PRIVATE(m)->current_music_id = -1;
	}

	PRIVATE(m)->current_music = music;
	switch(music) {
	case MB_MUSIC_SPLASH : {
		PRIVATE(m)->current_music_id = 
			mb_audio_engine_play_audio_file_full(PRIVATE(m)->engine,
							DATADIR"/monkey-bubble/sounds/splash.ogg",TRUE);
		break;
		}
	case MB_MUSIC_GAME : {
		PRIVATE(m)->current_music_id = 
			mb_audio_engine_play_audio_file_full(PRIVATE(m)->engine,
							DATADIR"/monkey-bubble/sounds/game.ogg",TRUE);
		break;
		}
	}
}

void sound_manager_play_fx(SoundManager *m,MbSample sample) 
{


  mb_audio_engine_play_audio_file(PRIVATE(m)->engine,PRIVATE(m)->samples_path[sample]);
}
