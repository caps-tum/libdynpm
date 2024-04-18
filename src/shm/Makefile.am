lib_LTLIBRARIES += libdynpmshm.la

libdynpmshm_la_SOURCES = \
						 src/shm/shm.c \
						 src/shm/pmix.c

libdynpmshm_la_LDFLAGS = -avoid-version -L${prefix}/lib/ -ldynpmcommon -lrt \
						 $(PMIX_LIBS)

libdynpmshm_la_LIBADD = libdynpmcommon.la

