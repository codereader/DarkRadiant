libspath = ../../libs
AM_CPPFLAGS = -I../../include -I$(libspath) $(GTK_CFLAGS) $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = entitylist.la

entitylist_la_LDFLAGS = -module -avoid-version $(GTK_LIBS) $(XML_LIBS)
entitylist_la_LIBADD = $(libspath)/gtkutil/libgtkutil.a \
					   $(libspath)/xmlutil/libxmlutil.a
entitylist_la_SOURCES = EntityListModule.cpp EntityList.cpp GraphTreeModel.cpp
