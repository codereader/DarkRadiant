#!/bin/sh
aclocal
libtoolize --force --copy
autoheader
autoconf
automake --add-missing --foreign

