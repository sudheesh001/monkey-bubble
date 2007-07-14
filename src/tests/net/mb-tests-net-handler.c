#include <unistd.h>
#include <stdio.h>
#include "mb-tests-net-utils.h"
#include <net/mb-net-server-handler.h>
#include <net/mb-net-connection.h>
#include <mb-tests-net-handler.h>

static void _test_server_handler();

gboolean mb_tests_net_handler_test_all()
{
	_test_server_handler();
	return TRUE;
}


static void _test_server_handler_ask_register_player();
static void _test_server_handler_register_player_response();
static void _test_server_handler_send_game_list();
static void _test_server_handler_send_ask_game_list();
static void _test_server_handler_send_create_game();
static void _test_server_handler_send_create_game_response();
static void _test_server_handler()
{
	_test_server_handler_ask_register_player();
	_test_server_handler_register_player_response();
	_test_server_handler_send_game_list();
	_test_server_handler_send_ask_game_list();
	_test_server_handler_send_create_game();
	_test_server_handler_send_create_game_response();
}

static MbNetPlayerHolder *_new_player()
{

	MbNetPlayerHolder *holder = g_new0(MbNetPlayerHolder, 1);
	holder->name = g_strdup("monkeybubble");
	return holder;
}


static MbNetGameListHolder *_new_game_list()
{

	MbNetGameListHolder *holder = g_new0(MbNetGameListHolder, 1);
	MbNetSimpleGameHolder *h;


	h = g_new0(MbNetSimpleGameHolder, 1);
	h->handler_id = 1;
	h->name = g_strdup("monkeybubble1");

	holder->games = g_list_append(holder->games, h);

	h = g_new0(MbNetSimpleGameHolder, 1);
	h->handler_id = 2;
	h->name = g_strdup("monkeybubble2");

	holder->games = g_list_append(holder->games, h);

	return holder;
}

static gboolean _test_game_list(MbNetGameListHolder * holder)
{
	gboolean ret = TRUE;
	ret &= (holder != NULL);
	ret &= (g_list_length(holder->games) == 2);
	return ret;
}

static gboolean _test_player(MbNetPlayerHolder * holder)
{
	gboolean ret = TRUE;
	ret &= (holder != NULL);
	ret &= (g_str_equal("monkeybubble", holder->name));
	return ret;
}


static void _ask_register_player(MbNetServerHandler * self,
				 MbNetConnection * con, guint32 handler_id,
				 MbNetPlayerHolder * holder,
				 TestSendReceive * tsr)
{

	if (_test_player(holder)) {
		tsr->sync->ret = TRUE;
	} else {
		tsr->sync->ret = FALSE;
	}

}

static void _test_server_handler_ask_register_player()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetServerHandler *handler;
	handler =
	    MB_NET_SERVER_HANDLER(g_object_new
				  (MB_NET_TYPE_SERVER_HANDLER, NULL));

	g_signal_connect(handler, "ask-register-player",
			 (GCallback) _ask_register_player, tsr);


	MbNetPlayerHolder *holder = _new_player();

	mb_net_server_handler_send_ask_register_player(handler, tsr->con2,
						       0, holder);

	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}

static void _register_player_response(MbNetServerHandler * self,
				      MbNetConnection * con,
				      guint32 handler_id,
				      MbNetPlayerHolder * holder,
				      gboolean ok, TestSendReceive * tsr)
{
	if (_test_player(holder)) {
		tsr->sync->ret = ok;
	} else {
		tsr->sync->ret = !ok;
	}

}

static void _test_server_handler_register_player_response()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetServerHandler *handler;
	handler =
	    MB_NET_SERVER_HANDLER(g_object_new
				  (MB_NET_TYPE_SERVER_HANDLER, NULL));

	g_signal_connect(handler, "register-player-response",
			 (GCallback) _register_player_response, tsr);


	MbNetPlayerHolder *holder = _new_player();

	mb_net_server_handler_send_register_player_response(handler,
							    tsr->con2, 0,
							    holder, TRUE);

	_wait_sync(tsr->sync);
	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);

	g_assert(tsr->sync->ret == TRUE);

	_begin_sync(tsr->sync);
	mb_net_server_handler_send_register_player_response(handler,
							    tsr->con2, 0,
							    holder, FALSE);

	_wait_sync(tsr->sync);
	mb_net_message_read_init(tsr->message, &s, &d, &a);

	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);

	g_assert(tsr->sync->ret == FALSE);

	_free_test_sendreceive(tsr);
}

static void _game_list(MbNetServerHandler * self,
		       MbNetConnection * con, guint32 handler_id,
		       MbNetGameListHolder * holder, TestSendReceive * tsr)
{
	if (_test_game_list(holder)) {
		tsr->sync->ret = TRUE;
	} else {
		tsr->sync->ret = FALSE;
	}

}

static void _test_server_handler_send_game_list()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetServerHandler *handler;
	handler =
	    MB_NET_SERVER_HANDLER(g_object_new
				  (MB_NET_TYPE_SERVER_HANDLER, NULL));

	g_signal_connect(handler, "game-list", (GCallback) _game_list,
			 tsr);


	MbNetGameListHolder *holder = _new_game_list();

	mb_net_server_handler_send_game_list(handler, tsr->con2, 0,
					     holder);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);

	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);

	g_assert(tsr->sync->ret == TRUE);

	_free_test_sendreceive(tsr);
}

static void _ask_game_list(MbNetServerHandler * self,
			   MbNetConnection * con,
			   guint32 handler_id, TestSendReceive * tsr)
{
	tsr->sync->ret = (handler_id == 10);

}

static void _test_server_handler_send_ask_game_list()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetServerHandler *handler;
	handler =
	    MB_NET_SERVER_HANDLER(g_object_new
				  (MB_NET_TYPE_SERVER_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "ask-game-list",
			 (GCallback) _ask_game_list, tsr);


	mb_net_server_handler_send_ask_game_list(handler, tsr->con2, 0);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);

	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);

	_free_test_sendreceive(tsr);
}



static void _create_game(MbNetServerHandler * self,
			 MbNetConnection * con,
			 guint32 handler_id, const gchar * name,
			 TestSendReceive * tsr)
{
	tsr->sync->ret = (handler_id == 10)
	    && (g_str_equal(name, "monkeybubble") == TRUE);

}

static void _test_server_handler_send_create_game()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetServerHandler *handler;
	handler =
	    MB_NET_SERVER_HANDLER(g_object_new
				  (MB_NET_TYPE_SERVER_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "create-game",
			 (GCallback) _create_game, tsr);


	mb_net_server_handler_send_create_game(handler, tsr->con2, 0,
					       "monkeybubble");
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);

	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);

	_free_test_sendreceive(tsr);
}

static void _create_game_response(MbNetServerHandler * self,
				  MbNetConnection * con,
				  guint32 handler_id, guint32 game_id,
				  TestSendReceive * tsr)
{
	tsr->sync->ret = (handler_id == 10) && (game_id == 1);

}

static void _test_server_handler_send_create_game_response()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetServerHandler *handler;
	handler =
	    MB_NET_SERVER_HANDLER(g_object_new
				  (MB_NET_TYPE_SERVER_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "create-game-response",
			 (GCallback) _create_game_response, tsr);


	mb_net_server_handler_send_create_game_response(handler, tsr->con2,
							0, 1);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);

	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);

	_free_test_sendreceive(tsr);
}
