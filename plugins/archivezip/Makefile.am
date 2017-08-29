AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(LIBSIGC_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = archivezip.la

archivezip_la_LDFLAGS = -module -avoid-version $(Z_LIBS) $(LIBSIGC_LIBS) $(FILESYSTEM_LIBS)
archivezip_la_SOURCES = ZipArchive.cpp plugin.cpp zlibstream.cpp

