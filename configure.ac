AC_INIT(libdynpm, m4_esyscmd_s([cat VERSION]), isaias.compres@tum.de)
AC_CONFIG_SRCDIR(src/)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE(subdir-objects)
AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([config.h])
AC_LANG([C])

LT_INIT

AC_PROG_CC
AM_PROG_LEX
AC_PROG_YACC

PKG_CHECK_MODULES([LIBEVENT], [libevent >= 2.0.0])

AC_OUTPUT(Makefile)
