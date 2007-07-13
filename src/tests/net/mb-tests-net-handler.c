#include <unistd.h>
#include <stdio.h>
#include "mb-tests-net-utils.h"
#include <net/mb-net-abstract-handler.h>
#include <net/mb-net-connection.h>
#include <mb-tests-net-handler.h>
/*
typedef struct _TestReceive TestReceive;


struct _TestReceive {

	TestSync *sync;
	MbNetAbstractHandler *h;
	MbNetConnection *con;
	MbNetConnection *con2;
};

static gboolean _test_new_connection(MbNetConnection * con,
				     MbNetConnection * new_con,
				     TestSync * sync);

static TestReceive *_init_receive_test(gpointer data2);
static void _free_receive_test(TestReceive * t);


static TestReceive *_init_receive_test(gpointer data2)
{
	MbNetAbstractHandler *h;
	MbNetConnection *con;
	MbNetConnection *con2;
	TestSync *sync;
	GError *error;

	error = NULL;
	TestReceive *r = g_new0(TestReceive, 1);

	sync = _init_sync();

	error = NULL;
	con =
	    MB_NET_CONNECTION(g_object_new(MB_NET_TYPE_CONNECTION, NULL));
	con2 =
	    MB_NET_CONNECTION(g_object_new(MB_NET_TYPE_CONNECTION, NULL));

	h = MB_NET_ABSTRACT_HANDLER(g_object_new
				    (MB_NET_TYPE_ABSTRACT_HANDLER, NULL));

	sync->data = h;
	sync->data2 = data2;
	sync->ret = FALSE;
	r->sync = sync;
	r->con = con;
	r->con2 = con2;
	r->h = h;
	g_signal_connect(con, "new-connection",
			 (GCallback) _test_new_connection, sync);
	mb_net_connection_accept_on(con, "mb://localhost:6600", &error);
	g_assert(error == NULL);

	mb_net_connection_connect(con2, "mb://localhost:6600", &error);
	g_assert(error == NULL);

	return r;
}


static void _free_receive_test(TestReceive * t)
{
	g_object_unref(t->h);
	mb_net_connection_stop(t->con, NULL);
	g_object_unref(t->con);
	g_object_unref(t->con2);


	_free_sync(t->sync);
}
static gboolean _test_new_connection(MbNetConnection * con,
				     MbNetConnection * new_con,
				     TestSync * sync)
{
	g_object_ref(new_con);
	g_signal_connect(new_con, "receive", (GCallback) sync->data2,
			 sync);
	mb_net_connection_listen(new_con, NULL);
	return FALSE;
}




static void _test_receive_int(MbNetConnection * con, guint32 size,
			      gpointer data, TestSync * sync)
{

	guint32 v1 = mb_netabstract_handler_read_int(data, size, 0);
	guint32 v2 =
	    mb_netabstract_handler_read_int(data, size, sizeof(guint32));
	g_print("receive int %d %d \n", v1, v2);

	if (v1 == 255 * 255 * 255 && v2 == 10)
		sync->ret = TRUE;
	_signal_sync(sync);
}



static void _test_sendreceive_int()
{
	GError *error;
	TestReceive *t;

	error = NULL;
	t = _init_receive_test(_test_receive_int);

	_begin_sync(t->sync);

	mb_net_abstract_handler_send_int(t->h, t->con2, 255 * 255 * 255,
					 10, &error);
	g_assert(error == NULL);

	_wait_sync(t->sync);

	g_assert(t->sync->ret == TRUE);

	_free_receive_test(t);
	//return TRUE;

}
*/
/*
static void _test_receive_xml(MbNetConnection * con, guint32 size,
			      gpointer data, TestSync * sync)
{

	xmlDoc *doc;
	g_print("read xml ... \n");
	doc =
	    mb_net_abstract_handler_read_xml_message(data, size,
						     sizeof(guint32) * 2);

	if (doc != NULL) {

		xmlNode *root;
		root = doc->children;

		g_assert(g_str_equal(root->name, "test1"));

		gchar *name =
		    (gchar *) xmlGetProp(root, (guchar *) "name");
		g_assert(name != NULL);
		g_assert(g_str_equal(name, "test2"));
		g_assert(g_str_equal
			 ((gchar *) root->children->content, "toto"));
		sync->ret = TRUE;
	}
	_signal_sync(sync);
}


static void _test_sendreceive_xml()
{
	GError *error;
	TestReceive *t;

	error = NULL;
	t = _init_receive_test(_test_receive_xml);

	xmlDoc *doc;
	xmlNode *text, *root;

	doc = xmlNewDoc((guchar *) "1.0");
	root = xmlNewNode(NULL, (guchar *) "test1");
	xmlDocSetRootElement(doc, root);

	xmlNewProp(root, (guchar *) "name", (guchar *) "test2");


	text = xmlNewText((const guchar *) "toto");

	xmlAddChild(root, text);

	_begin_sync(t->sync);

	mb_net_abstract_handler_send_xml_message(t->h, t->con2, 10, 20,
						 doc, &error);

	xmlFreeDoc(doc);

	g_assert(error == NULL);

	_wait_sync(t->sync);

	g_assert(t->sync->ret == TRUE);

	_free_receive_test(t);

}
*/
gboolean
mb_tests_net_handler_test_all ()
{
  //_test_sendreceive_int();
  //_test_sendreceive_xml();
  return TRUE;
}
