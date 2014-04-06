lib_LTLIBRARIES = libuvmemcached.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libuvmemcached.pc

include_HEADERS = \
    ../include/uvmemcached.h \
    ../include/private/pool.h

libuvmemcached_la_SOURCES = \
    memcached.c \
    pool.c

AM_CFLAGS = --pedantic -Wall -g -O3
AM_CPPFLAGS = -I$(top_srcdir)/include
bin_PROGRAMS = uvmemcached_selftest
uvmemcached_selftest_LDADD = libuvmemcached.la
uvmemcached_selftest_SOURCES = uvmemcached_selftest.c

libuvmemcached_la_LDFLAGS = -version-info @LTVER@

TESTS = uvmemcached_selftest
