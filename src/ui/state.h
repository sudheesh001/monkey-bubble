#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef MAEMO
#ifndef STATE_H
#define STATE_H

#include <glib.h>

gboolean state_load(void);
gboolean state_save(void);
void state_clear(void);

#endif

#endif
