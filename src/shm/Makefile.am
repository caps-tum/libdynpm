lib_LTLIBRARIES += libdynpmshm.la

libdynpmshm_la_SOURCES = \
						 src/shm/shm.c 

libdynpmshm_la_LDFLAGS = -avoid-version -L${prefix}/lib/ -ldynpmcommon -lrt 

libdynpmshm_la_LIBADD = libdynpmcommon.la

