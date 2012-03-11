#!/bin/sh
aclocal -I m4
libtoolize --force --copy
autoheader
autoconf
automake --add-missing --foreign

