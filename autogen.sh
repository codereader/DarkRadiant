#!/bin/bash
aclocal -I m4
# Use the glibtoolize command in OSX
case "$OSTYPE" in
  darwin*)  glibtoolize --force --copy ;; 
  *)        libtoolize --force --copy ;;
esac
autoheader
autoconf
automake --add-missing --copy --foreign

