# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# $Id: Makefile.am 22 2003-06-15 16:36:33Z lennart $

# This file is part of ifplugd.
#
# ifplugd is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# ifplugd is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ifplugd; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.

AC_PREREQ(2.57)
AC_INIT([ifplugd], [0.16], [mzvscyhtq@itaparica.org])
AC_CONFIG_SRCDIR([src/ifplugd.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# If using GCC specifiy some additional parameters
if test "x$GCC" = "xyes" ; then
   CFLAGS="$CFLAGS -pipe -Wall"
fi

if test -d /usr/local/stow ; then
   AC_MSG_NOTICE([*** Found /usr/local/stow: installing to /usr/local/stow/${PACKAGE_NAME}-${PACKAGE_VERSION} ***])
   AC_PREFIX_DEFAULT([/usr/local/stow/${PACKAGE_NAME}-${PACKAGE_VERSION}])
fi

AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([daemon], [daemon_fork],, [AC_MSG_ERROR([*** Sorry, you have to install libdaemon ***])])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_HEADER_TIME
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([alarm dup2 memset select setenv socket strchr strdup strerror strrchr strspn])

# LYNX documentation generation
AC_ARG_ENABLE(lynx,
        AC_HELP_STRING([--disable-lynx], [Turn off lynx usage for documentation generation]),
[case "${enableval}" in
  yes) lynx=yes ;;
  no)  lynx=no ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --disable-lynx) ;;
esac],[lynx=yes])

if test x$lynx = xyes ; then
   AC_CHECK_PROG(have_lynx, lynx, yes, no)

   if test x$have_lynx = xno ; then
     AC_MSG_ERROR([*** Sorry, you have to install lynx or use --disable-lynx ***])
   fi
fi

AM_CONDITIONAL([USE_LYNX], [test "x$lynx" = xyes])

# XMLTOMAN manpage generation
AC_ARG_ENABLE(xmltoman,
        AC_HELP_STRING([--disable-xmltoman], [Disable rebuilding of man pages with xmltoman]),
[case "${enableval}" in
  yes) xmltoman=yes ;;
  no)  xmltoman=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --disable-xmltoman]) ;;
esac],[xmltoman=yes])

if test x$xmltoman = xyes ; then
   AC_CHECK_PROG(have_xmltoman, xmltoman, yes, no)

   if test x$have_xmltoman = xno ; then
     AC_MSG_WARN([*** Not rebuilding man pages as xmltoman is not found ***])
     xmltoman=no
   fi
fi

AM_CONDITIONAL([USE_XMLTOMAN], [test "x$xmltoman" = xyes])

AC_CONFIG_FILES([src/Makefile Makefile man/Makefile conf/Makefile doc/Makefile doc/README.html patches/Makefile ifplugd.spec])
AC_OUTPUT
