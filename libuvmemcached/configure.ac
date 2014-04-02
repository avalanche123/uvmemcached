AC_PREREQ([2.60])
AC_INIT([uvmemcached], [0.1.0-dev], [mallluhuct@gmail.com])

AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)
AC_CONFIG_HEADERS([src/platform.h])

AM_INIT_AUTOMAKE([1.10 foreign])

# This defines PACKAGE_VERSION_... in src/platform.h
PV_MAJOR=`echo $PACKAGE_VERSION | cut -d . -f 1`
PV_MINOR=`echo $PACKAGE_VERSION | cut -d . -f 2`
PV_PATCH=`echo $PACKAGE_VERSION | cut -d . -f 3`
AC_DEFINE_UNQUOTED([PACKAGE_VERSION_MAJOR],[$PV_MAJOR],
    [UVMEMCACHED major version])
AC_DEFINE_UNQUOTED([PACKAGE_VERSION_MINOR],[$PV_MINOR],
    [UVMEMCACHED minor version])
AC_DEFINE_UNQUOTED([PACKAGE_VERSION_PATCH],[$PV_PATCH],
    [UVMEMCACHED patchlevel])
# This lets us use PACKAGE_VERSION in Makefiles
AC_SUBST(PACKAGE_VERSION)

LTVER="1:0:0"
AC_SUBST(LTVER)

AC_CONFIG_SRCDIR([src/memcached.c])

AC_PROG_CC
AC_PROG_LIBTOOL

# Allow libuv to be installed in a custom path:
AC_ARG_WITH([libuv],
            [AS_HELP_STRING([--with-libuv],
                            [Specify libuv prefix])],
            [uvmemcached_search_libuv="yes"],
            [])

if test "x$uvmemcached_search_libuv" = "xyes"; then
    if test -r "${with_libuv}/include/uv.h"; then
        CPPFLAGS="-I${with_libuv}/include ${CPPFLAGS}"
        LDFLAGS="-L${with_libuv}/lib ${LDFLAGS}"
    fi
fi

AC_ARG_WITH([libuv-include-dir],
            [AS_HELP_STRING([--with-libuv-include-dir],
                            [Specify libuv include prefix])],
            [uvmemcached_search_libuv_include="yes"],
            [])

if test "x$uvmemcached_search_libuv_include" = "xyes"; then
    if test -r "${with_libuv_include_dir}/uv.h"; then
        CPPFLAGS="-I${with_libuv_include_dir}/include ${CPPFLAGS}"
    fi
fi

AC_ARG_WITH([libuv_lib_dir],
            [AS_HELP_STRING([--with-libuv-lib-dir],
                            [Specify libuv library prefix])],
            [uvmemcached_search_libuv_lib="yes"],
            [])

if test "x$uvmemcached_search_libuv_lib" = "xyes"; then
    if test -r "${with_libuv_lib_dir}/libuv.{a|so|dylib}"; then
        LDFLAGS="-L${with_libuv}/lib ${LDFLAGS}"
    fi
fi


AC_CHECK_LIB(uv, uv_default_loop, ,[AC_MSG_ERROR([cannot link with -luv, install libuv.])])

AC_MSG_CHECKING([whether libuv installation works])
AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <uv.h>],
                                [uv_default_loop();])
               ],
               [AC_MSG_RESULT([yes])],
               [AC_MSG_ERROR([no. Please specify libuv installation prefix using --with-libuv])])

AC_HEADER_STDC
AC_CHECK_HEADERS(stdlib.h assert.h string.h stdio.h uv.h)

if test "x$GCC" = "xyes"; then
    CPPFLAGS="-pedantic -Werror -Wall ${CPPFLAGS}"
fi

AC_CONFIG_FILES([Makefile src/Makefile src/libuvmemcached.pc])
AC_OUTPUT
