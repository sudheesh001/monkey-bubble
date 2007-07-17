#include <unistd.h>
#include <stdio.h>
#include <net/mb-net-server.h>
#include <net/mb-net-client-server.h>
#include "mb-tests-net-utils.h"
#include "mb-tests-net-client-game.h"
#include <net/mb-net-holders.h>
static void _test_join();
static void _test_ask_player_list();
static void _test_ask_score();
gboolean mb_tests_net_client_game_test_all()
{
	GError *error;
	MbNetServer *s =
	    MB_NET_SERVER(g_object_new(MB_NET_TYPE_SERVER, NULL));

	error = NULL;
	mb_net_server_accept_on(s, "mb://localhost:6666", &error);
	g_assert(error == NULL);

	g_print("	_test_join\n");
	_test_join();
	g_print("	_test_ask_player_list\n");
	_test_ask_player_list();
	g_print("	_test_ask_score\n");
	_test_ask_score();
	mb_net_server_stop(s);
	g_object_unref(s);
	return TRUE;
}

static void _join_response(MbNetClientGame * game, gboolean ok,
			   TestSync * sync)
{
	sync->ret = ok;
	_signal_sync(sync);
}

static MbNetClientGame *_join_game(MbNetClientServer * client,
				   guint32 game_id)
{
	TestSync *sync = _init_sync();


	MbNetClientGame *client_game =
	    mb_net_client_server_create_client(client, game_id);

	g_signal_connect(client_game, "join-response",
			 (GCallback) _join_response, sync);

	_begin_sync(sync);
	mb_net_client_game_join(client_game);

	_wait_sync(sync);
	return client_game;
}

static void _test_join()
{
	TestSync *sync = _init_sync();

	_begin_sync(sync);

	MbNetClientServer *client =
	    mb_tests_net_client_server_connect("mb://localhost:6666",
					       "monkey1");
	MbNetClientGame *client_game =
	    mb_tests_net_client_server_create_game(client);
	g_signal_connect(client_game, "join-response",
			 (GCallback) _join_response, sync);
	mb_net_client_game_join(client_game);
	_wait_sync(sync);
	g_assert(sync->ret == TRUE);


	MbNetClientServer *client2 =
	    mb_tests_net_client_server_connect("mb://localhost:6666",
					       "monkey2");
	MbNetClientGame *client_game2 = _join_game(client2,
						   mb_net_client_game_get_game_id
						   (client_game));
	g_assert(client_game2 != NULL);

	mb_net_client_server_disconnect(client);
	g_object_unref(client);

	mb_net_client_server_disconnect(client2);
	g_object_unref(client2);

}

static void _player_list_changed(MbNetClientGame * g, TestSync * sync)
{
	sync->i = g_list_length(mb_net_client_game_get_players(g));
	_signal_sync(sync);
}


static void _test_ask_player_list()
{

	TestSync *sync = _init_sync();


	MbNetClientServer *client =
	    mb_tests_net_client_server_connect("mb://localhost:6666",
					       "monkey1");
	MbNetClientGame *client_game =
	    mb_tests_net_client_server_create_game(client);
	client_game = _join_game(client,
				 mb_net_client_game_get_game_id
				 (client_game));
	_begin_sync(sync);

	g_signal_connect(client_game, "player-list-changed",
			 (GCallback) _player_list_changed, sync);

	mb_net_client_game_ask_player_list(client_game);


	_wait_sync(sync);
	g_assert(sync->i == 1);
	MbNetClientServer *client2 =
	    mb_tests_net_client_server_connect("mb://localhost:6666",
					       "monkey2");
	MbNetClientGame *client_game2 = _join_game(client2,
						   mb_net_client_game_get_game_id
						   (client_game));
	g_assert(client_game2 != NULL);
	mb_net_client_game_ask_player_list(client_game);
	_wait_sync(sync);
	g_assert(sync->i == 2);

	mb_net_client_server_disconnect(client);
	g_object_unref(client);

	mb_net_client_server_disconnect(client2);
	g_object_unref(client2);

}

static void _score_changed(MbNetClientGame * g, TestSync * sync)
{
//      sync->i = g_list_length( mb_net_client_game_get_players(g));
	_signal_sync(sync);
}

static void _test_ask_score()
{
	TestSync *sync = _init_sync();


	MbNetClientServer *client =
	    mb_tests_net_client_server_connect("mb://localhost:6666",
					       "monkey1");
	MbNetClientGame *client_game =
	    mb_tests_net_client_server_create_game(client);
	client_game = _join_game(client,
				 mb_net_client_game_get_game_id
				 (client_game));
	_begin_sync(sync);

	g_signal_connect(client_game, "score-changed",
			 (GCallback) _score_changed, sync);
	mb_net_client_game_ask_score(client_game);
	_wait_sync(sync);
}
