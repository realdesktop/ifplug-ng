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

EXTRA_DIST=bootstrap.sh README LICENSE ifplugd.spec
SUBDIRS=src man conf doc patches

MAINTAINERCLEANFILES = README ifplugd.spec
noinst_DATA = README ifplugd.spec

README:
	rm -f README
	$(MAKE) -C doc README
	ln -s doc/README README

homepage:
	test -d $$HOME/homepage/lennart
	mkdir -p $$HOME/homepage/lennart/projects/ifplugd
	cp *.tar.gz $$HOME/homepage/lennart/projects/ifplugd
	cp doc/README.html doc/style.css doc/NEWS $$HOME/homepage/lennart/projects/ifplugd
	cp $$HOME/homepage/lennart/projects/ifplugd/README.html $$HOME/homepage/lennart/projects/ifplugd/index.html
