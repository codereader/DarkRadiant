libspath = ../../libs
AM_CPPFLAGS = -I../../include -I$(libspath)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = scenegraph.la

scenegraph_la_LIBADD = $(libspath)/scene/libscenegraph.a
scenegraph_la_LDFLAGS = -module -avoid-version
scenegraph_la_SOURCES = SceneGraph.cpp

