#include <unistd.h>
#include <stdio.h>
#include <net/mb-net-connection.h>

#include "mb-tests-net-utils.h"


void mb_net_connection_set_uri (MbNetConnection * self, const gchar * uri,
				GError ** error);
gint mb_net_connection_bind (MbNetConnection * self, const gchar * uri,
			     GError ** error);
void
_test_uri_eq (MbNetConnection * con, const gchar * host, guint port)
{

  /*g_assert( g_str_equal(con->host,host) == TRUE );
     g_assert( con->port == port); */
}

static gboolean
_test_set_uri ()
{
  GError *error;

  MbNetConnection *con;
  con = MB_NET_CONNECTION (g_object_new (MB_NET_TYPE_CONNECTION, NULL));

  error = NULL;
  mb_net_connection_set_uri (con, "mb://localhost:6100", &error);
  g_assert (error == NULL);
  _test_uri_eq (con, "localhost", 6100);

  mb_net_connection_set_uri (con, "mb://localhost", &error);
  g_assert (error == NULL);
  _test_uri_eq (con, "localhost", 6666);


  mb_net_connection_set_uri (con, "mb2://locahost", &error);
  g_assert (error != NULL);
  g_error_free (error);
  error = NULL;

  mb_net_connection_set_uri (con, "mb://locahost:54:55", &error);
  g_assert (error != NULL);
  g_error_free (error);
  error = NULL;

  g_object_unref (con);
  return TRUE;
}

void
_test_connect ()
{
  GError *error;

  MbNetConnection *con;
  con = MB_NET_CONNECTION (g_object_new (MB_NET_TYPE_CONNECTION, NULL));

  error = NULL;

  mb_net_connection_connect (con, "mb://www.google.com:80", &error);
  g_assert (error == NULL);


  mb_net_connection_connect (con, "mb://localhost:22", &error);
  g_assert (error == NULL);

  g_object_unref (con);
}

void
_test_bind ()
{
  GError *error;

  MbNetConnection *con;
  con = MB_NET_CONNECTION (g_object_new (MB_NET_TYPE_CONNECTION, NULL));

  error = NULL;
  gint sock;
  sock = mb_net_connection_bind (con, "mb://localhost:6666", &error);
  g_assert (error == NULL);
  g_assert (sock > 0);
  g_object_unref (con);
  close (sock);
}


static gboolean
_new_connection (MbNetConnection * con, MbNetConnection * new_con,
		 TestSendReceive * tsr)
{
  if (new_con != NULL)
    {
      tsr->sync->ret = TRUE;
      _signal_sync (tsr->sync);
    }
  else
    {
      g_error ("new connection is null ");
    }
  return FALSE;
}

static gboolean
_test_accept ()
{
  GError *error;

  error = NULL;

  TestSendReceive *tsr;

  tsr =
    _init_test_sendreceive ((GCallback) _new_connection, TRUE, NULL, NULL,
			    NULL);

  MbNetMessage *m = mb_net_message_create ();
  mb_net_message_add_string (m, "hello");
  mb_net_connection_send_message (tsr->con2, m, &error);
  g_assert (error == NULL);

  _wait_sync (tsr->sync);

  g_assert (tsr->sync->ret == TRUE);
  g_object_unref (m);
  _free_test_sendreceive (tsr);
  return TRUE;
}


static void
_test_receive_message (MbNetConnection * con, MbNetMessage * m,
		       TestSendReceive * tsr)
{
  g_object_ref (m);

  tsr->data3 = m;
  _signal_sync (tsr->sync);
}


static gboolean
_new_connection_and_listen_message (MbNetConnection * con,
				    MbNetConnection * new_con,
				    TestSendReceive * tsr)
{
  if (new_con != NULL)
    {
      g_object_ref (new_con);
      tsr->sync->data = new_con;
      g_signal_connect (new_con, "receive-message",
			(GCallback) _test_receive_message, tsr);
      mb_net_connection_listen (new_con, NULL);
    }
  else
    {
      g_error ("new connection is null ");
    }
  return FALSE;
}



static gboolean
_test_send_receive_message ()
{
  GError *error;

  error = NULL;

  TestSendReceive *tsr;

  tsr =
    _init_test_sendreceive ((GCallback) _new_connection_and_listen_message,
			    TRUE, NULL, NULL, NULL);

  MbNetMessage *m = mb_net_message_create ();
  mb_net_message_add_int (m, 1);
  mb_net_message_add_string (m, "hello");
  mb_net_message_add_boolean (m, FALSE);
  mb_net_message_add_string (m, "bye");

  mb_net_connection_send_message (tsr->con2, m, &error);
  g_assert (error == NULL);
  g_object_unref (m);

  _wait_sync (tsr->sync);

  g_assert (tsr->sync->data != NULL);
  g_object_unref (G_OBJECT (tsr->sync->data));
  g_assert (tsr->data3 != NULL);
  m = MB_NET_MESSAGE (tsr->data3);
  g_assert (mb_net_message_read_int (m) == 1);
  g_assert (g_str_equal (mb_net_message_read_string (m), "hello"));
  g_assert (mb_net_message_read_boolean (m) == FALSE);
  g_assert (g_str_equal (mb_net_message_read_string (m), "bye"));
  g_object_unref (m);
  _free_test_sendreceive (tsr);
  return TRUE;


}

gboolean
mb_tests_net_connection_test_all ()
{
  _test_set_uri ();
  _test_connect ();
  _test_bind ();
  _test_accept ();
  _test_send_receive_message ();
  return TRUE;
}
