AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(XML_CFLAGS) $(GTKMM_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = eclasstree.la

eclasstree_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.la
eclasstree_la_LDFLAGS = -module -avoid-version $(GTKMM_LIBS)
eclasstree_la_SOURCES = plugin.cpp EClassTreeBuilder.cpp EClassTree.cpp
