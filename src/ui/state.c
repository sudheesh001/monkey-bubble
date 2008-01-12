#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef MAEMO
#include <libosso.h>

#include "state.h"
#include "global.h"

struct StateData state;

gboolean state_load(void)
{
    osso_state_t osso_state;
    osso_return_t ret;

    osso_state.state_size = sizeof(struct StateData);
    osso_state.state_data = &state;

    ret = osso_state_read(global.osso, &osso_state);
    if (ret != OSSO_OK)
        return FALSE;
    return TRUE;
}

gboolean state_save(void)
{
    osso_state_t osso_state;
    osso_return_t ret;

    osso_state.state_size = sizeof(struct StateData);
    osso_state.state_data = &state;

    ret = osso_state_write(global.osso, &osso_state);

    if (ret != OSSO_OK)
        return FALSE;
    return TRUE;
}

void state_clear(void)
{
    state.game = 0;
    state.level = 0;
    state.score = 0;
    state.loadmap = 0;

    state_save();
}
#endif
