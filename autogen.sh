#!/bin/sh
aclocal
libtoolize
autoheader
autoconf
automake --add-missing --foreign

