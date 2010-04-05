AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs 
AM_CXXFLAGS = -fPIC

pkglib_LTLIBRARIES = libmath.la
libmath_la_LDFLAGS = -release @PACKAGE_VERSION@
libmath_la_SOURCES = matrix.cpp \
                     frustum.cpp \
                     aabb.cpp \
                     quaternion.cpp
