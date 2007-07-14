#include <gtk/gtk.h>
#include <mb-tests-net-connection.h>
#include <mb-tests-net-server.h>
#include <mb-tests-net-handler.h>
#include <mb-tests-net-message.h>


int main(int argc, char **argv)
{
	g_thread_init(NULL);
	g_type_init();
	mb_tests_net_message_test_all();
	mb_tests_net_connection_test_all();
	mb_tests_net_handler_test_all();
	mb_tests_net_server_test_all();

	g_print("all test OK\n");
	return 0;
}
