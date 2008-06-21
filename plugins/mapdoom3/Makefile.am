libspath = ../../libs
AM_CPPFLAGS = -I../../include -I$(libspath) $(XML_CFLAGS) $(GTK_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = mapdoom3.la

mapdoom3_la_LIBADD = $(libspath)/gtkutil/libgtkutil.a \
					 $(libspath)/xmlutil/libxmlutil.a
mapdoom3_la_LDFLAGS = -module -avoid-version $(GTK_LIBS) $(XML_LIBS)
mapdoom3_la_SOURCES = Doom3MapFormat.cpp \
                      NodeImporter.cpp \
                      InfoFile.cpp \
                      mapdoom3.cpp \
                      NodeExporter.cpp

