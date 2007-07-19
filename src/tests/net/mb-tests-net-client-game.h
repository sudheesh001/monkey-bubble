
#ifndef _MB_TESTS_NET__CLIENT_GAME_H
#define _MB_TESTS_NET__CLIENT_GAME_H
#include <net/mb-net-client-game.h>
#include <net/mb-net-client-server.h>
#include <mb-tests-net-client-server.h>

gboolean mb_tests_net_client_game_test_all();
MbNetClientGame *mb_tests_net_client_game_join(MbNetClientServer * client,
					       guint32 game_id);
guint32 mb_tests_net_client_game_start(MbNetClientGame * game);
#endif
