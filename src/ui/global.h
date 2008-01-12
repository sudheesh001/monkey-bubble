#ifdef MAEMO

#ifndef GLOBAL_H
#define GLOBAL_H

#include <libosso.h>

#include "game-1-player.h"

#define MONKEY_TEMP "/tmp/monkey_level_state"

struct GlobalData {
    osso_context_t *osso;
    Game1Player *game;
};

struct StateData {
    int game;
    int level;
    int score;
    int loadmap;
};

extern struct StateData state;
extern struct GlobalData global;

#endif

#endif
