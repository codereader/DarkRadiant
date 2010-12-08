AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(XML_CFLAGS) $(GLIB_CFLAGS) $(LIBSIGC_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = undo.la

undo_la_LDFLAGS = -module -avoid-version $(GTKMM_LIBS)
undo_la_SOURCES = UndoSystem.cpp

