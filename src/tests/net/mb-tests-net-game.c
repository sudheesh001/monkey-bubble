#include <unistd.h>
#include <stdio.h>

#include <net/mb-net-server.h>
#include <net/mb-net-game-handler.h>
#include <net/mb-net-connection.h>

#include "mb-tests-net-utils.h"
#include "mb-tests-net-server.h"
#include "mb-tests-net-game.h"


static void _init_game_test(TestSync ** ts, MbNetServer ** server,
			    MbNetConnection ** c,
			    MbNetGameHandler ** handler, guint32 * id);

static void _test_join();
static void _test_ask_player_list();
static void _test_start();
static void _test_stop();
static void _test_ask_score();

gboolean mb_tests_net_game_test_all()
{
	_test_join();
	_test_ask_player_list();
	_test_start();
	_test_stop();
	_test_ask_score();

	return TRUE;
}



static void _join_response(MbNetGameHandler * h, MbNetConnection * c,
			   guint32 handler_id, gboolean ok,
			   TestSync * sync)
{
	_signal_sync(sync);
}

static void _player_list(MbNetGameHandler * h, MbNetConnection * c,
			 guint32 handler_id,
			 MbNetPlayerListHolder * holder, TestSync * sync)
{
	_signal_sync(sync);
}

static void _test_join()
{

	MbNetServer *s;
	MbNetConnection *con;
	TestSync *sync;
	MbNetGameHandler *h;
	guint32 game_id;
	_init_game_test(&sync, &s, &con, &h, &game_id);
	g_signal_connect(h, "join-response",
			 (GCallback) _join_response, sync);
	g_signal_connect(h, "player-list", (GCallback) _player_list, sync);

	_begin_sync(sync);


	mb_net_game_handler_send_join(h, con, game_id, FALSE);

	_wait_sync(sync);
	_begin_sync(sync);

	mb_net_game_handler_send_ask_player_list(h, con, game_id);
	_wait_sync(sync);

	mb_net_server_stop(s);
	g_object_unref(s);
	mb_net_connection_stop(con, NULL);
	g_object_unref(con);
	g_object_unref(h);
}

static void _test_ask_player_list()
{
}

static void _test_start()
{
}

static void _test_stop()
{
}

static void _test_ask_score()
{
}




static void _message(MbNetConnection * con, MbNetMessage * m,
		     TestSync * sync)
{
	g_print("receive message \n");
	guint32 s, d, a;
	mb_net_message_read_init(m, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(sync->handler), con, s, d, a,
			       m);
}


static void _init_game_test(TestSync ** ts, MbNetServer ** server,
			    MbNetConnection ** c,
			    MbNetGameHandler ** handler, guint32 * id)
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

	g_signal_connect(con, "receive-message", (GCallback) _message,
			 sync);

	g_print("create game \n");
	guint32 game_id = mb_tests_net_server_create_game(con, sync);
	g_print("create game %d \n", game_id);
	MbNetGameHandler *h;
	h = MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));
	sync->handler = MB_NET_HANDLER(h);
	(*id) = game_id;

	(*ts) = sync;
	(*server) = s;
	(*c) = con;
	(*handler) = h;
}
