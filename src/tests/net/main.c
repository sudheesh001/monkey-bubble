#include <gtk/gtk.h>
#include <mb-tests-net-connection.h>
#include <mb-tests-net-server.h>
#include <mb-tests-net-game.h>
#include <mb-tests-net-server-handler.h>
#include <mb-tests-net-game-handler.h>
#include <mb-tests-net-match-handler.h>
#include <mb-tests-net-message.h>
#include <mb-tests-net-client-server.h>

int main(int argc, char **argv)
{
	g_thread_init(NULL);
	g_type_init();
	g_print("mb_tests_net_message_test_all();\n");
	mb_tests_net_message_test_all();
	g_print("mb_tests_net_connection_test_all();\n");
	mb_tests_net_connection_test_all();
	g_print("mb_tests_net_server_handler_test_all();\n");
	mb_tests_net_server_handler_test_all();
	g_print("mb_tests_net_game_handler_test_all();\n");
	mb_tests_net_game_handler_test_all();
	g_print("mb_tests_net_match_handler_test_all();\n");
	mb_tests_net_match_handler_test_all();
	g_print("mb_tests_net_server_test_all();\n");
	mb_tests_net_server_test_all();
	g_print("mb_tests_net_game_test_all();\n");
	mb_tests_net_game_test_all();
	g_print("mb_tests_net_client_server_test_all();\n");
	mb_tests_net_client_server_test_all();

	g_print("all test OK\n");

	return 0;
}
