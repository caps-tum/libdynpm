ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST =
AUTOMAKE_OPTIONS = foreign

include_HEADERS = \
				  include/dynpm_network.h \
				  include/dynpm_config.h \
				  include/dynpm_common.h \
				  include/dynpm_shm.h

AM_CPPFLAGS = \
			  -I$(top_srcdir)/include/ \
			  $(PMIX_CFLAGS) $(LIBEVENT_CFLAGS)

AM_CXXFLAGS = -fexceptions

dist_noinst_SCRIPTS = libdynpm.spec

sysconf_dir = $(sysconfdir)
dist_sysconf_DATA = dynpm.conf

EXTRA_DIST = dynpm.conf

bin_PROGRAMS =
lib_LTLIBRARIES =

# libraries (order matters)
include src/common/Makefile.am
include src/config/Makefile.am
include src/network/Makefile.am
include src/shm/Makefile.am
# programs
include src/print/Makefile.am
