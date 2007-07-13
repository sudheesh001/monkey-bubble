#include <unistd.h>
#include <stdio.h>

#include "mb-tests-net-utils.h"
#include <net/mb-net-server.h>
#include <net/mb-net-server-handler.h>
#include <net/mb-net-connection.h>

static void _test_ask_game_list();
gboolean mb_tests_net_server_test_all()
{
	_test_ask_game_list();
	return TRUE;
}



static void _message(MbNetConnection * con, MbNetMessage * m,
		     MbNetServerHandler * h)
{
	g_print("receive message \n");
	mb_net_handler_receive(MB_NET_HANDLER(h), con, m);
}

static void _game_list(MbNetServerHandler * self,
		       MbNetConnection * con,
		       MbNetGameListHolder * holder, TestSync * sync)
{

	g_print("print games list `\n");
	_signal_sync(sync);
}

static void _test_ask_game_list()
{
	GError *error;

	MbNetServer *s;

	MbNetConnection *con;
	TestSync *sync;

	sync = _init_sync();

	con =
	    MB_NET_CONNECTION(g_object_new(MB_NET_TYPE_CONNECTION, NULL));

	s = MB_NET_SERVER(g_object_new(MB_NET_TYPE_SERVER, NULL));

	error = NULL;
	mb_net_server_accept_on(s, "mb://localhost:6666", &error);
	g_assert(error == NULL);

	mb_net_connection_connect(con, "mb://localhost:6666", &error);
	g_assert(error == NULL);

	mb_net_connection_listen(con, &error);
	g_assert(error == NULL);


	MbNetServerHandler *h;
	h = MB_NET_SERVER_HANDLER(g_object_new
				  (MB_NET_TYPE_SERVER_HANDLER, NULL));

	g_signal_connect(con, "receive-message", (GCallback) _message, h);
	g_signal_connect(h, "game-list", (GCallback) _game_list, sync);

	_begin_sync(sync);

	mb_net_server_handler_send_ask_game_list(h, con, 1);

	_wait_sync(sync);
}
