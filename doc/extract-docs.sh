#!/bin/sh
#
# extract-docs.sh - Extract texinfo documentation from a source file.
#
# Usage: extract-docs.sh source.c >output.texi
#
# Copyright (C) 2004  Southern Storm Software, Pty Ltd.
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

in_doc=false
echo ''
echo "@c Extracted automatically from $1 - DO NOT EDIT"
echo ''
while read LINE ; do
	case "$LINE" in
		/\*@*)	in_doc=true ;;
		@\*/*)	echo ''
				in_doc=false ;;
		    *)	if test "$in_doc" = true ; then
					echo "$LINE"
				fi ;;
	esac
done < "$1" | sed -e '1,$s/^ *\* *//'

exit 0
