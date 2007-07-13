NULL=


INCLUDES = \
	$(MONKEYBUBBLE_CFLAGS) \
	-I$(top_srcdir)/src/ \
	-g -O2 -Wall -D_REENTRANT  -D_DEBUG_ -D_TIME_DEBUG_ \
	$(NULL)


bin_PROGRAMS = tests-net


tests_net_SOURCES = \
	main.c \
	mb-tests-net-connection.h mb-tests-net-connection.c \
	mb-tests-net-message.h mb-tests-net-message.c \
	mb-tests-net-handler.h mb-tests-net-handler.c \
	mb-tests-net-server.h mb-tests-net-server.c \
	mb-tests-net-utils.h mb-tests-net-utils.c \
	$(NULL)

tests_net_LDFLAGS =

tests_net_LDADD = \
	$(MONKEYBUBBLE_LIBS) \
	$(top_srcdir)/src/net/libmonkeynet.a \
	$(top_srcdir)/src/monkey/libmonkey.a \
	$(top_srcdir)/src/util/libmonkeyutil.a 


