AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(GLIB_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = vfspk3.la

vfspk3_la_LDFLAGS = -module -avoid-version $(GLIB_LIBS)
vfspk3_la_SOURCES = vfspk3.cpp Doom3FileSystem.cpp DirectoryArchive.cpp

