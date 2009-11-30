AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = scenegraph.la

scenegraph_la_LIBADD = $(top_builddir)/libs/scene/libscenegraph.la
scenegraph_la_LDFLAGS = -module -avoid-version
scenegraph_la_SOURCES = SceneGraph.cpp \
						Octree.cpp

