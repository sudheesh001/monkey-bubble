#include <unistd.h>
#include <stdio.h>
#include <net/mb-net-connection.h>

#include "mb-tests-net-utils.h"


void mb_net_connection_set_uri(MbNetConnection * self, const gchar * uri,
			       GError ** error);
gint mb_net_connection_bind(MbNetConnection * self, const gchar * uri,
			    GError ** error);
void _test_uri_eq(MbNetConnection * con, const gchar * host, guint port)
{

	/*g_assert( g_str_equal(con->host,host) == TRUE );
	   g_assert( con->port == port); */
}

static gboolean _test_set_uri()
{
	GError *error;

	MbNetConnection *con;
	con =
	    MB_NET_CONNECTION(g_object_new(MB_NET_TYPE_CONNECTION, NULL));

	error = NULL;
	mb_net_connection_set_uri(con, "mb://localhost:6100", &error);
	g_assert(error == NULL);
	_test_uri_eq(con, "localhost", 6100);

	mb_net_connection_set_uri(con, "mb://localhost", &error);
	g_assert(error == NULL);
	_test_uri_eq(con, "localhost", 6666);


	mb_net_connection_set_uri(con, "mb2://locahost", &error);
	g_assert(error != NULL);
	g_error_free(error);
	error = NULL;

	mb_net_connection_set_uri(con, "mb://locahost:54:55", &error);
	g_assert(error != NULL);
	g_error_free(error);
	error = NULL;

	g_object_unref(con);
	return TRUE;
}

void _test_connect()
{
	GError *error;

	MbNetConnection *con;
	con =
	    MB_NET_CONNECTION(g_object_new(MB_NET_TYPE_CONNECTION, NULL));

	error = NULL;

	mb_net_connection_connect(con, "mb://www.google.com:80", &error);
	g_assert(error == NULL);


	mb_net_connection_connect(con, "mb://localhost:22", &error);
	g_assert(error == NULL);

	g_object_unref(con);
}

void _test_bind()
{
	GError *error;

	MbNetConnection *con;
	con =
	    MB_NET_CONNECTION(g_object_new(MB_NET_TYPE_CONNECTION, NULL));

	error = NULL;
	gint sock;
	sock = mb_net_connection_bind(con, "mb://localhost:6666", &error);
	g_assert(error == NULL);
	g_assert(sock > 0);
	g_object_unref(con);
	close(sock);
}




static gboolean _test_accept()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, FALSE, NULL, NULL, NULL);

	_free_test_sendreceive(tsr);
	return TRUE;
}





static void
_message(MbNetConnection * con, MbNetMessage * m, TestSendReceive * tsr)
{
	g_object_ref(m);
	tsr->message = m;
	_signal_sync(tsr->sync);
}

static gboolean _test_send_receive_message()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr =
	    _init_test_sendreceive(NULL, (GCallback) _message, FALSE, NULL,
				   NULL, NULL);

	MbNetMessage *m = mb_net_message_create();
	mb_net_message_add_int(m, 1);
	mb_net_message_add_string(m, "hello");
	mb_net_message_add_boolean(m, FALSE);
	mb_net_message_add_string(m, "bye");
	_begin_sync(tsr->sync);

	mb_net_connection_send_message(tsr->con2, m, &error);
	g_assert(error == NULL);
	g_object_unref(m);

	_wait_sync(tsr->sync);

	g_assert(tsr->message != NULL);
	m = MB_NET_MESSAGE(tsr->message);
	g_assert(mb_net_message_read_int(m) == 1);
	g_assert(g_str_equal(mb_net_message_read_string(m), "hello"));
	g_assert(mb_net_message_read_boolean(m) == FALSE);
	g_assert(g_str_equal(mb_net_message_read_string(m), "bye"));
	_free_test_sendreceive(tsr);
	return TRUE;


}

static void _disconnected(MbNetConnection * con, TestSendReceive * tsr)
{
	tsr->sync->ret = TRUE;
	_signal_sync(tsr->sync);


}

static gboolean _test_disconnected()
{
	GError *error;

	error = NULL;

	TestSendReceive *tsr;

	tsr = _init_test_sendreceive(NULL, NULL, TRUE, NULL, NULL, NULL);

	g_signal_connect(tsr->con3, "disconnected",
			 (GCallback) _disconnected, tsr);
	mb_net_connection_stop(tsr->con2, NULL);

	_wait_sync(tsr->sync);

	g_assert(tsr->sync->ret == TRUE);
	_free_test_sendreceive(tsr);
	return TRUE;


}

gboolean mb_tests_net_connection_test_all()
{
	g_print("	_test_set_uri\n");
	_test_set_uri();
	g_print("	_test_connect\n");
	_test_connect();
	g_print("	_test_bind\n");
	_test_bind();
	g_print("	_test_accept\n");
	_test_accept();
	g_print("	_test_send_receive_message\n");
	_test_send_receive_message();
	g_print("	_test_disconnected\n");
	_test_disconnected();
	return TRUE;
}
