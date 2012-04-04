AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(GTKMM_CFLAGS)

modulesdir = $(pkglibdir)/plugins
modules_LTLIBRARIES = wavefront.la

wavefront_la_LDFLAGS = -module -avoid-version -Wl,-z,defs \
                       $(GTKMM_LIBS)
wavefront_la_SOURCES = plugin.cpp WaveFrontExporter.cpp WaveFrontModule.cpp

