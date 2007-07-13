#include <unistd.h>
#include <stdio.h>
#include <net/mb-net-server.h>
#include <net/mb-net-server-handler.h>
#include <net/mb-net-connection.h>


gboolean
mb_tests_net_server_test_all ()
{
  GError *error;

  MbNetServer *s;

  MbNetConnection *con;

  con = MB_NET_CONNECTION (g_object_new (MB_NET_TYPE_CONNECTION, NULL));

  s = MB_NET_SERVER (g_object_new (MB_NET_TYPE_SERVER, NULL));

  error = NULL;
  mb_net_server_accept_on (s, "mb://localhost:6666", &error);
  g_assert (error == NULL);

  mb_net_connection_connect (con, "mb://localhost:6666", &error);
  g_assert (error == NULL);

  MbNetServerHandler *h;
  h = MB_NET_SERVER_HANDLER (g_object_new (MB_NET_TYPE_SERVER_HANDLER, NULL));

  mb_net_server_handler_send_ask_game_list (h, con, 0);

  g_object_unref (con);
  g_object_unref (s);

  return TRUE;
}
