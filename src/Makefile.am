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

AM_CFLAGS = -DSYSCONFDIR="\"$(sysconfdir)\""

sbin_PROGRAMS = ifplugd ifstatus
ifplugd_SOURCES = ifplugd.c interface.c interface.h ethtool-kernel.h ethtool-local.h svn-revision.h wireless.15.h ifmonitor.h ifmonitor.c nlapi.h nlapi.c
ifstatus_SOURCES = ifstatus.c interface.c interface.h ethtool-kernel.h ethtool-local.h svn-revision.h wireless.15.h

MAINTAINERCLEANFILES = svn-revision.h

ifplugd.o: svn-revision.h
ifstatus.o: svn-revision.h

svn-revision.h: Makefile
	if test -d "$(top_srcdir)/.svn" ; then \
		if REV=`svn info "$(top_srcdir)" | grep ^Revision | cut -f2 -d" "` 2> /dev/null ; then \
			echo -e "#ifndef foosvnrevisionhfoo\n#define foosvnrevisionhfoo\n#define SVN_REVISION \"$$REV\"\n#endif" > $@ ; \
	    fi \
    else  \
		test -f $@ || touch $@ ; \
	fi
