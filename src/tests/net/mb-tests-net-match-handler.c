#include <unistd.h>
#include <stdio.h>
#include "mb-tests-net-utils.h"
#include <net/mb-net-match-handler.h>
#include <net/mb-net-connection.h>
#include <mb-tests-net-match-handler.h>


static void _test_penality();
static void _test_next_row();
static void _test_new_cannon_bubble();
static void _test_shoot();
static void _test_init_match();
static void _test_penality_bubbles();
static void _test_winlost();
static void _test_ready();
static void _test_start();


gboolean mb_tests_net_match_handler_test_all()
{


	_test_penality();
	_test_next_row();
	_test_new_cannon_bubble();
	_test_shoot();
	_test_init_match();
	_test_penality_bubbles();
	_test_winlost();
	_test_ready();
	_test_start();

	return TRUE;
}

static void _penality(MbNetMatchHandler * h, MbNetConnection * con,
		      guint32 hanlder_id, guint32 time, guint32 count,
		      TestSendReceive * tsr)
{
	g_assert(time == 10);
	g_assert(count == 20);
	tsr->sync->ret = TRUE;

}

static void _test_penality()
{
	TestSendReceive *tsr;
	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetMatchHandler *handler;
	handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "penality", (GCallback) _penality, tsr);

	mb_net_match_handler_send_penality(handler, tsr->con2, 0, 10, 20);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}

static void _next_row(MbNetMatchHandler * h, MbNetConnection * con,
		      guint32 hanlder_id, Color * bubbles,
		      TestSendReceive * tsr)
{
	int i;
	for (i = 0; i < 8; i++) {
		g_assert(bubbles[i] == i);
	}

	tsr->sync->ret = TRUE;
}


static void _test_next_row()
{
	Color colors[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetMatchHandler *handler;
	handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "next-row", (GCallback) _next_row, tsr);

	mb_net_match_handler_send_next_row(handler, tsr->con2, 0, colors);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}

static void _new_cannon_bubble(MbNetMatchHandler * h,
			       MbNetConnection * con, guint32 hanlder_id,
			       Color bubble, TestSendReceive * tsr)
{
	g_assert(bubble == 1);
	tsr->sync->ret = TRUE;

}


static void _test_new_cannon_bubble()
{
	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetMatchHandler *handler;
	handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "new-cannon-bubble",
			 (GCallback) _new_cannon_bubble, tsr);

	mb_net_match_handler_send_new_cannon_bubble(handler, tsr->con2, 0,
						    1);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}


static void _shoot(MbNetMatchHandler * h, MbNetConnection * con,
		   guint32 hanlder_id, guint32 time, gfloat radian,
		   TestSendReceive * tsr)
{
	g_assert(time == 20);
	g_assert(radian == 0.5);
	tsr->sync->ret = TRUE;

}
static void _test_shoot()
{
	TestSendReceive *tsr;
	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetMatchHandler *handler;
	handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "shoot", (GCallback) _shoot, tsr);

	mb_net_match_handler_send_shoot(handler, tsr->con2, 0, 20, 0.5);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}

static void _match_init(MbNetMatchHandler * h, MbNetConnection * con,
			guint32 hanlder_id, guint32 count, Color * bubbles,
			gboolean odd, guint32 bubble1, guint32 bubble2,
			TestSendReceive * tsr)
{
	g_assert(count == 255);
	int i;
	for (i = 0; i < 255; i++) {
		g_assert(bubbles[i] == i % 8);
	}

	g_assert(odd == TRUE);
	g_assert(bubble1 == 1);
	g_assert(bubble2 == 2);
	tsr->sync->ret = TRUE;

}
static void _test_init_match()
{
	Color colors[255];
	int i;
	for (i = 0; i < 255; i++) {
		colors[i] = i % 8;
	}

	TestSendReceive *tsr;
	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetMatchHandler *handler;
	handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "match_init", (GCallback) _match_init,
			 tsr);

	mb_net_match_handler_send_match_init(handler, tsr->con2, 0, 255,
					     colors, TRUE, 1, 2);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}



static void _penality_bubbles(MbNetMatchHandler * h, MbNetConnection * con,
			      guint32 hanlder_id, Color * bubbles,
			      TestSendReceive * tsr)
{
	int i;
	for (i = 0; i < 7; i++) {
		g_assert(bubbles[i] == i);
	}
	tsr->sync->ret = TRUE;

}


static void _test_penality_bubbles()
{
	Color colors[7] = { 0, 1, 2, 3, 4, 5, 6 };
	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetMatchHandler *handler;
	handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "penality-bubbles",
			 (GCallback) _penality_bubbles, tsr);

	mb_net_match_handler_send_penality_bubbles(handler, tsr->con2, 0,
						   colors);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}


static void _winlost(MbNetMatchHandler * h, MbNetConnection * con,
		     guint32 hanlder_id, gboolean win,
		     TestSendReceive * tsr)
{
	g_assert(win == TRUE);
	tsr->sync->ret = TRUE;

}
static void _test_winlost()
{
	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetMatchHandler *handler;
	handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "winlost", (GCallback) _winlost, tsr);

	mb_net_match_handler_send_winlost(handler, tsr->con2, 0, TRUE);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}

static void _ready(MbNetMatchHandler * h, MbNetConnection * con,
		   guint32 hanlder_id, guint32 player_id,
		   TestSendReceive * tsr)
{
	tsr->sync->ret = TRUE;
	g_assert(player_id == 1);

}

static void _test_ready()
{
	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetMatchHandler *handler;
	handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "ready", (GCallback) _ready, tsr);

	mb_net_match_handler_send_ready(handler, tsr->con2, 0, 1);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}

static void _simple(MbNetMatchHandler * h, MbNetConnection * con,
		    guint32 hanlder_id, TestSendReceive * tsr)
{
	tsr->sync->ret = TRUE;

}

static void _test_start()
{
	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	MbNetMatchHandler *handler;
	handler =
	    MB_NET_MATCH_HANDLER(g_object_new
				 (MB_NET_TYPE_MATCH_HANDLER, NULL));
	mb_net_handler_set_id(MB_NET_HANDLER(handler), 10);
	g_signal_connect(handler, "start", (GCallback) _simple, tsr);

	mb_net_match_handler_send_start(handler, tsr->con2, 0);
	_wait_sync(tsr->sync);

	guint32 s, d, a;
	mb_net_message_read_init(tsr->message, &s, &d, &a);
	mb_net_handler_receive(MB_NET_HANDLER(handler), tsr->con2, s, d, a,
			       tsr->message);
	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
}
