# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# Copyright (c) 2013, Groupon, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# Neither the name of GROUPON nor the names of its contributors may be
# used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

AC_PREREQ(2.59)
AC_INIT([roll], [1.0])
AC_CONFIG_SRCDIR([src/roll.c])
AC_CONFIG_HEADER([config.h])
ostype=`uname -s`

# ==== Check for programs ====================================================
CFLAGS="${CFLAGS=}"
CFLAGS="$CFLAGS -Wall -Werror -g"
AC_PROG_CC
AC_PROG_AWK

# ==== How to do static linking? =============================================
AC_MSG_CHECKING(how to enable static linking for certain libraries)
if test "x$ostype" != "xDarwin"; then
        ldversion=`ld -V 2>&1 | head -1`
fi
if echo $ldversion | egrep "GNU|Solaris" > /dev/null; then
    LD_START_STATIC="-Wl,-Bstatic"
    LD_END_STATIC="-Wl,-Bdynamic"
    AC_MSG_RESULT(GNU or Solaris)
else
    LD_START_STATIC=""
    LD_END_STATIC=""
    AC_MSG_RESULT([linking everything dynamically])
fi

# ==== Check for libraries ===================================================
AC_CHECK_FILE([/usr/local/lib], LDFLAGS="$LDFLAGS -L/usr/local/lib")
AC_CHECK_FILE([/usr/local/ssl/lib], LDFLAGS="$LDFLAGS -L/usr/local/ssl/lib")

AC_CHECK_LIB([yaml], [yaml_parser_initialize], DEPS_LIBS="$DEPS_LIBS -lyaml")
AC_CHECK_LIB([curl], [curl_easy_init], DEPS_LIBS="$DEPS_LIBS -lcurl")

# If linking statically, we need curl dependencies
if test "x$LD_START_STATIC" != "x"; then
    AC_CHECK_LIB([ssl], [SSL_new], CURL_LIBS="$CURL_LIBS -lssl",, "-lcrypto")
    AC_CHECK_LIB([crypto], [SHA1_Init], CURL_LIBS="$CURL_LIBS -lcrypto")
    AC_CHECK_LIB([z], [gzwrite], CURL_LIBS="$CURL_LIBS -lz")
    AC_CHECK_LIB([cares], [ares_gethostbyname], CURL_LIBS="$CURL_LIBS -lcares")
    AC_CHECK_LIB([idn], [idna_strerror], CURL_LIBS="$CURL_LIBS -lidn")
    AC_DEFINE([CURL_STATICLIB])
fi

AC_CHECK_LIB([rt], [sched_setscheduler], BASE_LIBS="$BASE_LIBS -lrt")
AC_CHECK_LIB([dl], [dlopen], BASE_LIBS="$BASE_LIBS -ldl")
AC_CHECK_LIB([pthread], [pthread_create], BASE_LIBS="$BASE_LIBS -lpthread")

LIBS="$LIBS $LD_START_STATIC $DEPS_LIBS $CURL_LIBS $LD_END_STATIC $BASE_LIBS"

# ==== Check for headers =====================================================
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([stdlib.h string.h unistd.h fcntl.h limits.h netdb.h sys/socket.h])

# ==== Check for typedefs, structures, and compiler characteristics ==========
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_STRUCT_TIMEZONE

# ==== Check for library functions = =========================================
AC_FUNC_FORK
AC_CHECK_FUNCS([dup2 localtime_r memset setenv clearenv gethostname mkdir ftruncate strerror])
AC_CHECK_FUNCS([strlcpy strcspn strdup strstr])

# ==== Output ===============================================================
# Output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
