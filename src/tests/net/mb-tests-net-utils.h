
#ifndef _MB_TESTS_NET__UTILS_H
#define _MB_TESTS_NET__UTILS_H

#include <gtk/gtk.h>
#include <net/mb-net-connection.h>

typedef struct _TestSync TestSync;
typedef struct _TestSendReceive TestSendReceive;

struct _TestSync
{

  GMutex *m;
  GCond *cond;
  gpointer data;
  gpointer data2;
  gboolean ret;
};

struct _TestSendReceive
{

  TestSync *sync;
  MbNetConnection *con;
  MbNetConnection *con2;
  gpointer data3;
  GCallback connect_method;
};


TestSync *_init_sync ();

void _free_sync (TestSync * sync);
void _begin_sync (TestSync * sync);

void _wait_sync (TestSync * sync);

void _signal_sync (TestSync * sync);


TestSendReceive *_init_test_sendreceive (GCallback func, gboolean locked,
					 gpointer data, gpointer data2,
					 gpointer data3);
void _free_test_sendreceive (TestSendReceive * t);

#endif
