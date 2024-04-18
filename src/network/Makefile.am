lib_LTLIBRARIES += libdynpmnetwork.la

libdynpmnetwork_la_SOURCES = \
							src/network/client.c \
							src/network/server.c \
							src/network/queue.c \
							src/network/tbon.c \
							src/network/protocol.c

libdynpmnetwork_la_LDFLAGS = -avoid-version \
							-L${prefix}/lib/ -ldynpmcommon -ldynpmconfig \
							$(LIBEVENT_LIBS) -lpthread

libdynpmnetwork_la_LIBADD = libdynpmcommon.la libdynpmconfig.la

