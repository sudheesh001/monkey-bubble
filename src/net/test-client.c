#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

#include <gtk/gtk.h>

#include "mn-game-manager.h"
#include "monkey-network-game.h"
#include "monkey-message-handler.h"

void recv_message(MonkeyMessageHandler * mmh,
		  guint32 client_id,
		  gchar * message,
		  gpointer * p) {

  g_log(G_LOG_DOMAIN,G_LOG_LEVEL_DEBUG,"message recieved client_id %d, message %s\n",client_id,message);
  
}
                                                                                

int main(int argc, char **argv) {

  int sock;
  struct sockaddr_in sock_client;
  struct hostent *src_host;
  MonkeyMessageHandler * handler;

  gtk_init(&argc,&argv);
  g_thread_init(NULL);
  
  sock =  socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  
 
  if ( sock == -1) {
    perror("socket()");
    return 0;
  }


  bzero((char *) &sock_client, sizeof(sock_client));
  sock_client.sin_family = AF_INET;
  sock_client.sin_port = (unsigned short) htons(6666);
  src_host = (struct hostent *) gethostbyname("localhost");	
  if (!src_host) {
    fprintf(stderr, "Not a valid Server IP...\n");
    return 0;
  }

  bcopy( (char *) src_host->h_addr, (char *) &sock_client.sin_addr.s_addr, src_host->h_length);
  
  while (connect(sock, (struct sockaddr *) &sock_client, sizeof(sock_client)) == -1) {
    if (errno != EAGAIN)
      {
	perror("connect()");
	return 0;
      }
  }

  handler = monkey_message_handler_new(sock);

   
  g_signal_connect( G_OBJECT(handler),
		    "recv-message",
		    G_CALLBACK( recv_message ),
		    NULL);

  monkey_message_handler_start_listening(handler);

  monkey_message_handler_join( handler);
  
  return 0;
}
