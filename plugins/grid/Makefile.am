AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(XML_CFLAGS) $(GTKMM_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = grid.la

grid_la_LDFLAGS = -module -avoid-version $(GTKMM_LIBS)
grid_la_SOURCES = Grid.cpp

