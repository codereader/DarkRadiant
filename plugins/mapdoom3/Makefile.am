AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
               $(XML_CFLAGS) $(GTKMM_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = mapdoom3.la

mapdoom3_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.la \
                     $(top_builddir)/libs/xmlutil/libxmlutil.la
mapdoom3_la_LDFLAGS = -module -avoid-version $(GTKMM_LIBS) $(XML_LIBS)
mapdoom3_la_SOURCES = Doom3MapFormat.cpp \
                 Doom3PrefabFormat.cpp \
                      NodeImporter.cpp \
                      InfoFile.cpp \
                      mapdoom3.cpp \
                      NodeExporter.cpp \
                 primitiveparsers/BrushDef.cpp \
                 primitiveparsers/BrushDef3.cpp \
                 primitiveparsers/Patch.cpp \
                 primitiveparsers/PatchDef2.cpp \
                 primitiveparsers/PatchDef3.cpp

