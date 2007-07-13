#include <unistd.h>
#include <stdio.h>
#include "mb-tests-net-utils.h"
#include <net/mb-net-connection.h>




TestSync *_init_sync()
{

	TestSync *sync = g_new0(TestSync, 1);

	sync->m = g_mutex_new();
	sync->cond = g_cond_new();
	sync->data = NULL;
	return sync;
}

void _free_sync(TestSync * sync)
{
	g_mutex_free(sync->m);
	g_cond_free(sync->cond);

	g_free(sync);
}

void _begin_sync(TestSync * sync)
{
	g_mutex_lock(sync->m);
}


void _wait_sync(TestSync * sync)
{
	GTimeVal timeout;
	g_get_current_time(&timeout);
	g_cond_wait(sync->cond, sync->m);
	g_mutex_unlock(sync->m);
}

void _signal_sync(TestSync * sync)
{
	g_mutex_lock(sync->m);
	g_cond_signal(sync->cond);
	g_mutex_unlock(sync->m);
}


void
_init_receive_message_no_action(MbNetConnection * con, MbNetMessage * m,
				TestSendReceive * tsr)
{

}


static void
_init_basic_receive_message(MbNetConnection * con, MbNetMessage * m,
			    TestSendReceive * tsr)
{
	g_object_ref(m);
	tsr->message = m;
	_signal_sync(tsr->sync);
}


static gboolean
_init_basic_new_connection(MbNetConnection * con,
			   MbNetConnection * new_con,
			   TestSendReceive * tsr)
{
	if (new_con != NULL) {
		g_object_ref(new_con);
		tsr->sync->data = new_con;
		g_signal_connect(new_con, "receive-message",
				 (GCallback) tsr->new_message, tsr);
		mb_net_connection_listen(new_con, NULL);
	} else {
		g_error("new connection is null ");
	}
	return FALSE;
}


TestSendReceive *_init_test_sendreceive(GCallback func,
					GCallback receive_message,
					gboolean locked, gpointer data,
					gpointer data2, gpointer data3)
{

	MbNetConnection *con;
	MbNetConnection *con2;
	TestSync *sync;
	GError *error;

	error = NULL;
	TestSendReceive *r = g_new0(TestSendReceive, 1);

	sync = _init_sync();

	if (locked) {
		g_mutex_lock(sync->m);
	}
	error = NULL;
	con =
	    MB_NET_CONNECTION(g_object_new(MB_NET_TYPE_CONNECTION, NULL));
	con2 =
	    MB_NET_CONNECTION(g_object_new(MB_NET_TYPE_CONNECTION, NULL));

	sync->data = data;
	sync->data2 = data2;
	sync->ret = FALSE;

	r->sync = sync;
	r->con = con;
	r->con2 = con2;
	r->data3 = data3;

	if (func == NULL) {
		func = _init_basic_new_connection;
	}
	r->connect_method = func;
	if (receive_message == NULL) {
		receive_message = _init_basic_receive_message;
	}

	r->new_message = receive_message;
	g_signal_connect(con, "new-connection", (GCallback) func, r);
	mb_net_connection_accept_on(con, "mb://localhost:6600", &error);
	g_assert(error == NULL);

	mb_net_connection_connect(con2, "mb://localhost:6600", &error);
	g_assert(error == NULL);

	return r;
}


void _free_test_sendreceive(TestSendReceive * t)
{
	mb_net_connection_stop(t->con, NULL);
	g_object_unref(t->con);
	g_object_unref(t->con2);

	_free_sync(t->sync);
}
