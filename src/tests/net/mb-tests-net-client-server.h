
#ifndef _MB_TESTS_NET__CLIENT_SERVER_H
#define _MB_TESTS_NET__CLIENT_SERVER_H
#include <net/mb-net-client-game.h>
#include <net/mb-net-client-server.h>
gboolean mb_tests_net_client_server_test_all();

MbNetClientServer *mb_tests_net_client_server_connect(const gchar * uri,
						      const gchar * name);
MbNetClientGame *mb_tests_net_client_server_create_game(MbNetClientServer *
							client);
GList *mb_tests_net_client_server_get_games(MbNetClientServer * client);

#endif
