#!/bin/sh
#
# mkpdf - Make the PDF version of texinfo documentation

exec texi2dvi --pdf libjit.texi
