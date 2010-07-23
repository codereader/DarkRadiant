AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(GTKMM_CFLAGS) $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = entitylist.la

entitylist_la_LDFLAGS = -module -avoid-version $(GTKMM_LIBS) $(XML_LIBS)
entitylist_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.la \
                       $(top_builddir)/libs/xmlutil/libxmlutil.la
entitylist_la_SOURCES = EntityListModule.cpp EntityList.cpp GraphTreeModel.cpp
