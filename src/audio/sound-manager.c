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
#include <canberra.h>

enum
{
  PLAY_ID_0,
  PLAY_ID_FX,
  PLAY_ID_MUSIC
};

#define PRIVATE(sound_manager) (sound_manager->private)

static GObjectClass* parent_class = NULL;

struct SoundManagerPrivate
{
  ca_context* context;
  gchar     **samples_path;
  int       * sample_ids;
  gint        current_music_id;
  MbMusic     current_music;
  gboolean    active;
};


static SoundManager* sound_manager_new (void);
static void          load_samples      (SoundManager* m);

static void
sound_manager_instance_init (SoundManager* sound_manager)
{
  int result;

  sound_manager->private =g_new0 (SoundManagerPrivate, 1);

  result = ca_context_create (&PRIVATE (sound_manager)->context);
  if (result != CA_SUCCESS)
    {
      g_warning ("ca_context_create(): %d", result);
    }
  result = ca_context_open (PRIVATE (sound_manager)->context);
  if (result != CA_SUCCESS)
    {
      g_warning ("ca_context_open(): %d", result);
    }
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
#if 0
  mb_audio_engine_cache_audio_file(  PRIVATE(m)->engine,
				     PRIVATE(m)->samples_path[i]);
#endif
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

static gboolean
replay_music (gpointer  user_data)
{
  /* FIXME: there is no proper bookkeeping */
  sound_manager_play_music (user_data, PRIVATE (SOUND_MANAGER (user_data))->current_music_id);

  return FALSE;
}

static void
finished_cb (ca_context* c,
             guint32     id,
             int         error_code,
             gpointer    user_data)
{
  if (error_code != CA_ERROR_CANCELED)
    {
      g_idle_add (replay_music, user_data);
    }
}

void
sound_manager_play_music (SoundManager* m,
                          MbMusic       music)
{
  g_return_if_fail (IS_SOUND_MANAGER (m));

  if (PRIVATE (m)->current_music_id != NO_MUSIC)
    {
      int result = ca_context_cancel (PRIVATE (m)->context, PLAY_ID_MUSIC);

      if (result != CA_SUCCESS)
        {
          g_warning ("ca_context_cancel(): %d", result);
        }
    }

  PRIVATE (m)->current_music_id = music;

  if (PRIVATE (m)->current_music_id != NO_MUSIC)
    {
      gchar const* paths[] =
        {
          DATADIR "/monkey-bubble/sounds/splash.ogg",
          DATADIR "/monkey-bubble/sounds/game.ogg"
        };
      ca_proplist* properties = NULL;
      int result = ca_proplist_create (&properties);
      if (result != CA_SUCCESS)
        {
          g_warning ("ca_proplist_create(): %d", result);
        }
      result = ca_proplist_sets (properties,
                                 CA_PROP_MEDIA_FILENAME,
                                 paths[PRIVATE (m)->current_music_id]);
      if (result != CA_SUCCESS)
        {
          g_warning ("ca_proplist_sets(): %d", result);
        }
      result = ca_context_play_full (PRIVATE (m)->context,
                                     PLAY_ID_MUSIC,
                                     properties,
                                     finished_cb,
                                     m);
      if (result != CA_SUCCESS)
        {
          g_warning ("ca_context_play(): %d", result);
        }

      result = ca_proplist_destroy (properties);
      if (result != CA_SUCCESS)
        {
          g_warning ("ca_proplist_destroy(): %d", result);
        }
    }
}

void sound_manager_play_fx (SoundManager* m,
                            MbSample      sample)
{
  int result = ca_context_play (PRIVATE (m)->context,
                                PLAY_ID_FX,
                                CA_PROP_MEDIA_FILENAME, PRIVATE (m)->samples_path[sample],
                                NULL);

  if (result != CA_SUCCESS)
    {
      g_print ("ca_context_play(): %d\n", result);
    }
}

/* vim:set et sw=2 cino=t0,f0,(0,{s,>2s,n-1s,^-1s,e2s: */
