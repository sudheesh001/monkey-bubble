
#ifndef _MB_TESTS_NET__SERVER_H
#define _MB_TESTS_NET__SERVER_H

#include <net/mb-net-connection.h>
#include "mb-tests-net-utils.h"
gboolean mb_tests_net_server_test_all();
guint32 mb_tests_net_server_create_game(MbNetConnection * con,
					TestSync * s);

#endif
