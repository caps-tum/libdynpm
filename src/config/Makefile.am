BUILT_SOURCES = src/config/bison.h
AM_YFLAGS = -d
AM_LFLAGS =
AM_CPPFLAGS += -DPREFIX='"$(prefix)"'\
			   -I$(top_srcdir)/src/common/ \
			   -I$(top_builddir)/src/config/

lib_LTLIBRARIES += libdynpmconfig.la

libdynpmconfig_la_SOURCES = \
							src/config/flex.l \
							src/config/bison.y \
							src/config/config.c \
							src/config/load.c
libdynpmconfig_la_LDFLAGS = -avoid-version
libdynpmconfig_la_LIBADD  =

