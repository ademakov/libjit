#!/bin/sh
#
# mkhtml.sh - Make html documentation from Texinfo input.
#
# Usage: mkhtml outdir

# Check that we are executed in the correct directory.
if [ ! -f libjit.texi ]; then
	echo "Cannot find libjit.texi"
	exit 1
fi

# Get the full pathname of the input file.
PATHNAME=`pwd`/libjit.texi

# Change to the output directory and execute "texi2html".
exec texi2html "$PATHNAME"
