#!/bin/sh
#
# auto_gen.sh - Make the Makefile.in and configure files.
#
# Copyright (C) 2001, 2002  Southern Storm Software, Pty Ltd.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# Run aclocal to update the macros.
aclocal

# Run autoheader to generate config.h.in.
autoheader

# Get extra options to use depending upon the automake version.
AM_VERSION=`automake --version`
case "$AM_VERSION" in
    automake*1.4*) AM_FLAGS="" ;;
                *) AM_FLAGS="--ignore-deps" ;;
esac

# Run libtoolize if necessary.
if test ! -f "ltconfig" ; then
	libtoolize --copy 2>/dev/null
fi

# Run automake and autoconf.
automake --add-missing --copy $AM_FLAGS
autoconf
exit 0
