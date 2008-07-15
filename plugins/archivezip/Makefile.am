AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = archivezip.la

archivezip_la_LDFLAGS = -module -avoid-version $(Z_LIBS)
archivezip_la_SOURCES = ZipArchive.cpp pkzip.cpp plugin.cpp zlibstream.cpp

