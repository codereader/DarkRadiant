AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
			  $(GTK_CFLAGS) $(XML_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_editing.la

dm_editing_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.la \
						  $(top_builddir)/libs/xmlutil/libxmlutil.la
dm_editing_la_LDFLAGS = -module -avoid-version $(GTK_LIBS) $(XML_LIBS)
dm_editing_la_SOURCES = plugin.cpp \
						AIHeadPropertyEditor.cpp
