/*
 * Copyright/Licensing information.
 */

#ifndef MB_AUDIO_ENGINE_H
#define MB_AUDIO_ENGINE_H


#include <glib-object.h>

/*
 * Type macros.
 */
#define MB_TYPE_AUDIO_ENGINE           (mb_audio_engine_get_type ())
#define MB_AUDIO_ENGINE(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), MB_TYPE_AUDIO_ENGINE, MbAudioEngine))


typedef struct _MbAudioEngine MbAudioEngine;
typedef struct _MbAudioEngineClass MbAudioEngineClass;

struct _MbAudioEngine {
  GObject object;
  /* instance members */

};

struct _MbAudioEngineClass {
  GObjectClass parent;
  /* class members */
  /* signals */
};

/* used by MB_AUDIO_ENGINE_TYPE */
GType mb_audio_engine_get_type (void);

/*
 * Method definitions.
 */

MbAudioEngine * mb_audio_engine_new(void);

void mb_audio_engine_cache_audio_file(MbAudioEngine * self,
				      const gchar * path);


int mb_audio_engine_play_audio_file(MbAudioEngine * self,
				    const gchar * path);

void mb_audio_engine_stop(MbAudioEngine * self,gint file_id);
#endif
