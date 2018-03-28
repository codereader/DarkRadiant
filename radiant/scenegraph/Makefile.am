AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(LIBSIGC_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = scenegraph.la

scenegraph_la_LIBADD = $(top_builddir)/libs/math/libmath.la \
                       $(top_builddir)/libs/scene/libscenegraph.la
scenegraph_la_LDFLAGS = -module -avoid-version $(LIBSIGC_LIBS)
scenegraph_la_SOURCES = SceneGraph.cpp \
						SceneGraphFactory.cpp \
						Octree.cpp

