#include <unistd.h>
#include <stdio.h>
#include "mb-tests-net-utils.h"
#include <net/mb-net-game-handler.h>
#include <net/mb-net-connection.h>
#include <mb-tests-net-game-handler.h>

static void _test_game_handler_join();
static void _test_game_handler_join_response();
static void _test_game_handler_ask_player_list();
static void _test_game_handler_player_list();
static void _test_game_handler_match_created();
static void _test_game_handler_start();
static void _test_game_handler_stop();
static void _test_game_handler_ask_score();
static void _test_game_handler_score();


gboolean mb_tests_net_game_handler_test_all()
{
	_test_game_handler_join();
	_test_game_handler_join_response();
	_test_game_handler_ask_player_list();
	_test_game_handler_player_list();
	_test_game_handler_match_created();
	_test_game_handler_start();
	_test_game_handler_stop();
	_test_game_handler_ask_score();
	_test_game_handler_score();
	return TRUE;
}


static void _score(MbNetGameHandler * handler, MbNetConnection * con,
		   guint32 handler_id, MbNetScoreHolder * holder,
		   TestSendReceive * tsr)
{
	g_assert(holder != NULL);
	g_assert(g_list_length(holder->score_by_player) == 2);
	MbNetPlayerScoreHolder *h =
	    (MbNetPlayerScoreHolder *) holder->score_by_player->data;
	g_assert(g_str_equal(h->name, "monkey"));
	g_assert(h->score == 2);

	h = (MbNetPlayerScoreHolder *) g_list_next(holder->
						   score_by_player)->data;
	g_assert(g_str_equal(h->name, "bubble"));
	g_assert(h->score == 10);
	tsr->sync->ret = TRUE;
}

static MbNetPlayerScoreHolder *_create_player_score(const gchar * name,
						    guint32 score)
{
	MbNetPlayerScoreHolder *h =
	    (MbNetPlayerScoreHolder *) g_new0(MbNetPlayerScoreHolder, 1);
	h->name = g_strdup(name);
	h->score = score;
	return h;
}

static void _test_game_handler_score()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetGameHandler *handler;
	handler =
	    MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "score", (GCallback) _score, tsr);

	MbNetScoreHolder *holder;
	holder = (MbNetScoreHolder *) g_new0(MbNetScoreHolder, 1);
	holder->score_by_player =
	    g_list_append(holder->score_by_player,
			  _create_player_score("monkey", 2));
	holder->score_by_player =
	    g_list_append(holder->score_by_player,
			  _create_player_score("bubble", 10));
	mb_net_game_handler_send_score(handler, tsr->con2, 0, holder);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);

}

static void _ask_score(MbNetGameHandler * handler, MbNetConnection * con,
		       guint32 handler_id, TestSendReceive * tsr)
{
	g_assert(handler_id == 10);
	tsr->sync->ret = TRUE;
}

static void _test_game_handler_ask_score()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetGameHandler *handler;
	handler =
	    MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "ask-score",
			 (GCallback) _ask_score, tsr);


	mb_net_game_handler_send_ask_score(handler, tsr->con2, 0);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}


static void _start_stop(MbNetGameHandler * handler, MbNetConnection * con,
			guint32 handler_id, TestSendReceive * tsr)
{

	g_assert(handler_id == 10);
	tsr->sync->ret = TRUE;
}

static void _test_game_handler_start()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetGameHandler *handler;
	handler =
	    MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "start", (GCallback) _start_stop, tsr);


	mb_net_game_handler_send_start(handler, tsr->con2, 0);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);

	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}

static void _test_game_handler_stop()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetGameHandler *handler;
	handler =
	    MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "stop", (GCallback) _start_stop, tsr);


	mb_net_game_handler_send_stop(handler, tsr->con2, 0);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);

	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}

static void _match_created(MbNetGameHandler * handler,
			   MbNetConnection * con, guint32 handler_id,
			   guint32 game_id, TestSendReceive * tsr)
{

	g_assert(handler_id == 10);
	g_assert(game_id == 20);
	tsr->sync->ret = TRUE;
}

static void _test_game_handler_match_created()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetGameHandler *handler;
	handler =
	    MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "match-created",
			 (GCallback) _match_created, tsr);


	mb_net_game_handler_send_match_created(handler, tsr->con2, 0, 20);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);

	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}


static void _join(MbNetGameHandler * handler, MbNetConnection * con,
		  guint32 handler_id, guint32 player_id,
		  gboolean has_observer, TestSendReceive * tsr)
{
	g_assert(has_observer == TRUE);
	g_assert(handler_id == 10);
	g_assert(player_id == 20);
	tsr->sync->ret = TRUE;
}

static void _test_game_handler_join()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetGameHandler *handler;
	handler =
	    MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "join", (GCallback) _join, tsr);


	mb_net_game_handler_send_join(handler, tsr->con2, 0, 20, TRUE);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}


static void _join_response(MbNetGameHandler * handler,
			   MbNetConnection * con, guint32 handler_id,
			   gboolean ok, TestSendReceive * tsr)
{
	g_assert(ok == TRUE);
	g_assert(handler_id == 10);
	tsr->sync->ret = TRUE;
}

static void _test_game_handler_join_response()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetGameHandler *handler;
	handler =
	    MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "join-response",
			 (GCallback) _join_response, tsr);


	mb_net_game_handler_send_join_response(handler, tsr->con2, 0,
					       TRUE);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}


static void _ask_player_list(MbNetGameHandler * handler,
			     MbNetConnection * con, guint32 handler_id,
			     TestSendReceive * tsr)
{
	g_assert(handler_id == 10);
	tsr->sync->ret = TRUE;
}

static void _test_game_handler_ask_player_list()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetGameHandler *handler;
	handler =
	    MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "ask-player-list",
			 (GCallback) _ask_player_list, tsr);


	mb_net_game_handler_send_ask_player_list(handler, tsr->con2, 0);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}


static MbNetPlayerHolder *_create_player(const gchar * name)
{
	MbNetPlayerHolder *h;
	h = g_new0(MbNetPlayerHolder, 1);
	h->name = g_strdup(name);
	return h;
}

static void player_list(MbNetGameHandler * handler, MbNetConnection * con,
			guint32 handler_id, MbNetPlayerListHolder * holder,
			TestSendReceive * tsr)
{
	g_assert(handler_id == 10);
	g_assert(holder != NULL);
	g_assert(g_list_length(holder->players) == 2);
	MbNetPlayerHolder *h = (MbNetPlayerHolder *) holder->players->data;
	g_assert(g_str_equal(h->name, "monkey"));
	h = (MbNetPlayerHolder *) g_list_next(holder->players)->data;
	g_assert(g_str_equal(h->name, "bubble"));
	tsr->sync->ret = TRUE;
}

static void _test_game_handler_player_list()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetGameHandler *handler;
	handler =
	    MB_NET_GAME_HANDLER(g_object_new
				(MB_NET_TYPE_GAME_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "player-list",
			 (GCallback) player_list, tsr);

	MbNetPlayerListHolder *holder = NULL;
	holder = g_new0(MbNetPlayerListHolder, 1);
	holder->players =
	    g_list_append(holder->players, _create_player("monkey"));
	holder->players =
	    g_list_append(holder->players, _create_player("bubble"));
	mb_net_game_handler_send_player_list(handler, tsr->con2, 0,
					     holder);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}
