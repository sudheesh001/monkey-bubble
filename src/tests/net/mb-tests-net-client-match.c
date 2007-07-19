#include <unistd.h>
#include <stdio.h>
#include <net/mb-net-server.h>
#include <net/mb-net-client-server.h>
#include "mb-tests-net-utils.h"
#include "mb-tests-net-client-game.h"
#include "mb-tests-net-client-match.h"
#include <net/mb-net-holders.h>

static void _test_ready();
gboolean mb_tests_net_client_match_test_all()
{
	GError *error;
	MbNetServer *s =
	    MB_NET_SERVER(g_object_new(MB_NET_TYPE_SERVER, NULL));

	error = NULL;
	GMainLoop *loop = g_main_loop_new(NULL, TRUE);
	g_thread_create((GThreadFunc) g_main_loop_run, loop, TRUE, NULL);
	mb_net_server_accept_on(s, "mb://localhost:6666", &error);
	g_assert(error == NULL);

	_test_ready();
	mb_net_server_stop(s);
	g_print("serfer stopped \n");
	g_object_unref(s);
	return TRUE;
}

static void _bubble_sticked(Board * board, Bubble * bubble, gint stiked,
			    TestSync * sync)
{
	_signal_sync(sync);
}

static void _match_start(MbNetClientMatch * match, TestSync * sync)
{
	mb_net_client_match_lock(match);
	Monkey *m = mb_net_client_match_get_monkey(match);
	Board *b = playground_get_board(monkey_get_playground(m));
	g_assert(board_bubbles_count(b) == 30);
	g_signal_connect(b, "bubble-sticked", (GCallback) _bubble_sticked,
			 sync);
	mb_net_client_match_unlock(match);

	mb_net_client_match_shoot(match);


}

static void _generic_start(MbNetClientGame * g, MbNetClientMatch * match,
			   TestSync * sync)
{
	mb_net_client_match_ready(match);
	g_signal_connect(match, "start", (GCallback) _match_start, sync);
}
static void _test_ready()
{


	TestSync *sync = _init_sync();
	TestSync *sync2 = _init_sync();
	MbNetClientServer *client =
	    mb_tests_net_client_server_connect("mb://localhost:6666",
					       "monkey1");
	MbNetClientGame *client_game =
	    mb_tests_net_client_server_create_game(client);

	guint32 game_id = mb_net_client_game_get_game_id(client_game);
	client_game = mb_tests_net_client_game_join(client, game_id);
	g_signal_connect(client_game, "start", (GCallback) _generic_start,
			 sync);
	MbNetClientServer *client2 =
	    mb_tests_net_client_server_connect("mb://localhost:6666",
					       "monkey2");
	MbNetClientGame *client_game2 = NULL;
	client_game2 = mb_tests_net_client_game_join(client2, game_id);
	g_signal_connect(client_game2, "start", (GCallback) _generic_start,
			 sync2);


	_begin_sync(sync);
	_begin_sync(sync2);

	mb_net_client_game_start(client_game);


	_wait_sync(sync);
	_wait_sync(sync2);


}
