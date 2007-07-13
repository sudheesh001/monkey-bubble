#include <unistd.h>
#include <stdio.h>
#include "mb-tests-net-utils.h"
#include <mb-tests-net-message.h>
#include <net/mb-net-message.h>




static void
_test_sendreceive_int ()
{
  MbNetMessage *m = mb_net_message_create ();
  mb_net_message_add_int (m, 255 * 254 * 253);

  guint32 r = mb_net_message_read_int (m);
  g_assert (r == 255 * 254 * 253);
  g_object_unref (m);
}

static void
_test_sendreceive_boolean ()
{
  MbNetMessage *m = mb_net_message_create ();
  mb_net_message_add_boolean (m, TRUE);
  mb_net_message_add_boolean (m, FALSE);
  mb_net_message_add_boolean (m, TRUE);
  gboolean b1 = mb_net_message_read_boolean (m);
  gboolean b2 = mb_net_message_read_boolean (m);
  gboolean b3 = mb_net_message_read_boolean (m);
  g_assert (b1 == TRUE);
  g_assert (b2 == FALSE);
  g_assert (b3 == TRUE);

  g_object_unref (m);
}

static void
_test_sendreceive_string ()
{
  MbNetMessage *m = mb_net_message_create ();
  mb_net_message_add_string (m, "foo");
  mb_net_message_add_string (m, "monkeybubble");
  gchar *s1 = mb_net_message_read_string (m);
  gchar *s2 = mb_net_message_read_string (m);
  g_assert (g_str_equal (s1, "foo") == TRUE);
  g_assert (g_str_equal (s2, "monkeybubble") == TRUE);

  g_free (s1);
  g_free (s2);
  g_object_unref (m);
}

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
mb_tests_net_message_test_all ()
{
  _test_sendreceive_int ();
  _test_sendreceive_boolean ();
  _test_sendreceive_string ();

  //_test_sendreceive_xml();
  return TRUE;
}
