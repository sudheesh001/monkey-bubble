
#ifndef _MB_TESTS_NET__UTILS_H
#define _MB_TESTS_NET__UTILS_H

#include <gtk/gtk.h>
#include <net/mb-net-connection.h>
#include <net/mb-net-message.h>
#include <net/mb-net-handler.h>
typedef struct _TestSync TestSync;
typedef struct _TestSendReceive TestSendReceive;

struct _TestSync {

	GMutex *m;
	GCond *cond;
	gpointer data;
	gpointer data2;
	gboolean ret;
	gboolean ret2;
	guint32 id;
	MbNetHandler *handler;
};

struct _TestSendReceive {

	TestSync *sync;
	MbNetConnection *con;
	MbNetConnection *con2;
	MbNetConnection *con3;
	gpointer data3;
	GCallback connect_method;
	GCallback new_message;
	MbNetMessage *message;

	gpointer temp;
};


TestSync *_init_sync();

void _free_sync(TestSync * sync);
void _begin_sync(TestSync * sync);

void _wait_sync(TestSync * sync);

void _signal_sync(TestSync * sync);


void _init_receive_message_no_action(MbNetConnection * con,
				     MbNetMessage * m,
				     TestSendReceive * tsr);
TestSendReceive *_init_test_sendreceive(GCallback new_connection,
					GCallback new_message,
					gboolean locked, gpointer data,
					gpointer data2, gpointer data3);
void _free_test_sendreceive(TestSendReceive * t);

#endif
