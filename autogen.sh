#!/bin/bash
aclocal -I m4
# Use the glibtoolize command in OSX
if [ "$OSTYPE" == "darwin"* ]; then
	glibtoolize --force --copy
else
	libtoolize --force --copy
fi
autoheader
autoconf
automake --add-missing --copy --foreign

