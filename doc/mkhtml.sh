#!/bin/sh
#
# mkhtml.sh - Make html documentation from Texinfo input.
#
# Usage: mkhtml outdir

# Check the command-line.
if [ -z "$1" ]; then
	echo "Usage: $0 outdir"
	exit 1
fi

# Check that we are executed in the correct directory.
if [ ! -f libjit.texi ]; then
	echo "Cannot find libjit.texi"
	exit 1
fi

# Create the output directory.
if [ ! -d "$1" ]; then
	mkdir "$1"
fi

# Get the full pathname of the input file.
PATHNAME=`pwd`/libjit.texi

# Change to the output directory and execute "texi2html".
cd "$1"
exec texi2html -split_chapter "$PATHNAME"
