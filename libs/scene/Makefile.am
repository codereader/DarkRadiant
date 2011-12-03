AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(LIBSIGC_CFLAGS)

pkglib_LTLIBRARIES = libscenegraph.la
libscenegraph_la_LDFLAGS = -release @PACKAGE_VERSION@ $(LIBSIGC_LIBS)
libscenegraph_la_LIBADD = $(top_builddir)/libs/math/libmath.la
libscenegraph_la_SOURCES = InstanceWalkers.cpp \
			 			  TraversableNodeSet.cpp \
						  Node.cpp
