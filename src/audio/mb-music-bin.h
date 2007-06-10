/*
 * Copyright/Licensing information.
 */

#ifndef MB_MUSIC_BIN_H
#define MB_MUSIC_BIN_H


#include <glib-object.h>

/*
 * Type macros.
 */
#define MB_TYPE_MUSIC_BIN           (mb_music_bin_get_type ())
#define MB_MUSIC_BIN(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), MB_TYPE_MUSIC_BIN, MbMusicBin))


typedef struct _MbMusicBin MbMusicBin;
typedef struct _MbMusicBinClass MbMusicBinClass;

struct _MbMusicBin {
  GObject object;
  /* instance members */

};

struct _MbMusicBinClass {
  GObjectClass parent;
  /* class members */
  /* signals */
};

/* used by MB_MUSIC_BIN_TYPE */
GType mb_music_bin_get_type (void);

/*
 * Method definitions.
 */

MbMusicBin * mb_music_bin_new(const gchar * binname,const gchar * path);
GstElement * mb_music_bin_get_element(MbMusicBin * self);

#endif
