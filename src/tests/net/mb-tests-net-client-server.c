#include <unistd.h>
#include <stdio.h>

#include <net/mb-net-server.h>
#include <net/mb-net-client-server.h>
#include "mb-tests-net-utils.h"
#include "mb-tests-net-client-server.h"

static void _test_connect();
static void _ask_games();
static void _create_game();

gboolean mb_tests_net_client_server_test_all()
{
	GError *error;
	MbNetServer *s =
	    MB_NET_SERVER(g_object_new(MB_NET_TYPE_SERVER, NULL));

	error = NULL;
	mb_net_server_accept_on(s, "mb://localhost:6666", &error);
	g_assert(error == NULL);

	_test_connect();
	_create_game();
	_ask_games();

	mb_net_server_stop(s);
	g_object_unref(s);
	return TRUE;
}


static void _test_connect()
{

	MbNetClientServer *client = NULL;
	client = mb_tests_net_client_server_connect("mb://localhost:6666");
	g_assert(client != NULL);
	mb_net_client_server_disconnect(client);
	g_object_unref(client);


}


static void _create_game()
{

	MbNetClientServer *client = NULL;
	client = mb_tests_net_client_server_connect("mb://localhost:6666");
	MbNetClientGame *game =
	    mb_tests_net_client_server_create_game(client);
	g_assert(game != NULL);
	mb_net_client_server_disconnect(client);
	g_object_unref(game);
	g_object_unref(client);

}


static void _ask_games()
{
	MbNetClientServer *client = NULL;
	client = mb_tests_net_client_server_connect("mb://localhost:6666");

	GList *games = mb_tests_net_client_server_get_games(client);
	g_assert(g_list_length(games) == 0);

	MbNetClientGame *game =
	    mb_tests_net_client_server_create_game(client);
	games = mb_tests_net_client_server_get_games(client);
	g_assert(g_list_length(games) == 1);


	mb_net_client_server_disconnect(client);
	g_object_unref(game);
	g_object_unref(client);
}

static void _new_game_list(MbNetClientServer * client, TestSync * sync)
{

	sync->ret = TRUE;
	_signal_sync(sync);

}

GList *mb_tests_net_client_server_get_games(MbNetClientServer * client)
{
	GError *error;
	TestSync *sync;

	sync = _init_sync();
	error = NULL;
	error = NULL;


	gulong i = g_signal_connect(client, "new-game-list",
				    (GCallback) _new_game_list, sync);
	_begin_sync(sync);
	mb_net_client_server_ask_games(client, &error);
	g_assert(error == NULL);
	_wait_sync(sync);
	g_assert(sync->ret == TRUE);
	g_signal_handler_disconnect(client, i);

	return mb_net_client_server_get_games(client);
}


static void _game_created(MbNetClientServer * s, MbNetClientGame * game,
			  TestSync * sync)
{
	g_assert(game != NULL);
	g_object_ref(game);
	sync->data = game;
	sync->ret = TRUE;
	_signal_sync(sync);
}

MbNetClientGame *mb_tests_net_client_server_create_game(MbNetClientServer *
							client)
{
	GError *error;
	TestSync *sync;

	sync = _init_sync();
	error = NULL;


	gulong i = g_signal_connect(client, "game-created",
				    (GCallback) _game_created, sync);

	_begin_sync(sync);
	mb_net_client_server_create_game(client, "bubble game", &error);
	g_assert(error == NULL);

	_wait_sync(sync);
	g_signal_handler_disconnect(client, i);

	g_assert(sync->ret == TRUE);
	return MB_NET_CLIENT_GAME(sync->data);
}



static void _connected(MbNetClientServer * client, gboolean ok,
		       TestSync * sync)
{
	g_assert(ok);
	sync->ret = ok;
	_signal_sync(sync);
}

MbNetClientServer *mb_tests_net_client_server_connect(const gchar * uri)
{
	GError *error;
	TestSync *sync;

	sync = _init_sync();
	error = NULL;

	MbNetClientServer *client =
	    MB_NET_CLIENT_SERVER(g_object_new
				 (MB_NET_TYPE_CLIENT_SERVER, NULL));
	mb_net_client_server_set_name(client, "monkeybubble");
	int i =
	    g_signal_connect(client, "connected", (GCallback) _connected,
			     sync);

	_begin_sync(sync);
	mb_net_client_server_connect(client, uri, &error);


	g_assert(error == NULL);

	_wait_sync(sync);

	g_assert(sync->ret == TRUE);
	_free_sync(sync);

	g_signal_handler_disconnect(client, i);

	return client;


}
