#include <unistd.h>
#include <stdio.h>

#include "mb-tests-net-utils.h"
#include <net/mb-net-server.h>
#include <net/mb-net-server-handler.h>
#include <net/mb-net-connection.h>

static void _init_server_test(TestSync ** ts, MbNetServer ** server,
			      MbNetConnection ** c,
			      MbNetServerHandler ** handler);
static void _test_ask_game_list();
static void _test_ask_register_player();
gboolean mb_tests_net_server_test_all()
{
	_test_ask_register_player();
	_test_ask_game_list();
	return TRUE;
}




static void _new_player(MbNetServer * server, MbNetServerPlayer * p,
			TestSync * sync)
{
	g_print("new player  ! %s \n", p->name);
	sync->ret2 = TRUE;
	_signal_sync(sync);

}

static void _register_player_response(MbNetServerHandler * self,
				      MbNetConnection * con,
				      guint32 handler,
				      MbNetPlayerHolder * holder,
				      gboolean ok, TestSync * sync)
{
	g_print("register player response\n");
	sync->ret2 = TRUE;
	_signal_sync(sync);
}


static void _test_ask_register_player()
{
	//GError *error;
	MbNetServer *s;
	MbNetConnection *con;
	TestSync *sync;
	TestSync *sync2;

	MbNetServerHandler *h;

	_init_server_test(&sync, &s, &con, &h);
	sync2 = _init_sync();
	g_signal_connect(h, "register-player-response",
			 (GCallback) _register_player_response, sync);

	g_signal_connect(s, "new-player", (GCallback) _new_player, sync2);
	sync->ret2 = FALSE;
	_begin_sync(sync);

	_begin_sync(sync2);

	MbNetPlayerHolder *holder = g_new0(MbNetPlayerHolder, 1);
	holder->name = g_strdup("monkeybubble");

	mb_net_server_handler_send_ask_register_player(h, con, 0, holder);

	_wait_sync(sync2);
	_wait_sync(sync);

	g_assert(sync->ret2 == TRUE);
	g_assert(sync2->ret2 == TRUE);

	mb_net_connection_stop(con, NULL);
	g_object_unref(con);
	mb_net_server_stop(s);
	g_object_unref(s);
	g_object_unref(h);

}

static void _game_list(MbNetServerHandler * self,
		       MbNetConnection * con, guint32 handler,
		       MbNetGameListHolder * holder, TestSync * sync)
{
	_signal_sync(sync);
}


static void _test_ask_game_list()
{
	//GError *error;
	MbNetServer *s;
	MbNetConnection *con;
	TestSync *sync;
	MbNetServerHandler *h;

	_init_server_test(&sync, &s, &con, &h);
	g_signal_connect(h, "game-list", (GCallback) _game_list, sync);

	_begin_sync(sync);

	mb_net_server_handler_send_ask_game_list(h, con, 0);

	_wait_sync(sync);

	mb_net_server_stop(s);
	g_object_unref(s);
	mb_net_connection_stop(con, NULL);
	g_object_unref(con);
	g_object_unref(h);
}




static void _message(MbNetConnection * con, MbNetMessage * m,
		     MbNetServerHandler * h)
{
	g_print("receive message \n");
	guint32 s, d, a;
	mb_net_message_read_init(m, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(h), con, s, d, a, m);
}


static void _init_server_test(TestSync ** ts, MbNetServer ** server,
			      MbNetConnection ** c,
			      MbNetServerHandler ** handler)
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


	(*ts) = sync;
	(*server) = s;
	(*c) = con;
	(*handler) = h;
}
