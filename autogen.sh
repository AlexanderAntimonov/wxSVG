#!/bin/sh
case `uname` in
  Darwin*) glibtoolize --copy || exit 1 ;;
  *) libtoolize --copy || exit 1 ;;
esac
aclocal -I m4
automake --foreign -a -c
autoconf
