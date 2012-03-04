AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(GLIB_CFLAGS) $(LIBSIGC_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = filetypes.la

filetypes_la_LDFLAGS = -module -avoid-version $(GTKMM_LIBS) $(LIBSIGC_LIBS)
filetypes_la_SOURCES = FileTypeRegistry.cpp

