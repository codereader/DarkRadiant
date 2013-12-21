AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
               $(XML_CFLAGS) $(GTKMM_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = mapdoom3.la

mapdoom3_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.la \
                     $(top_builddir)/libs/xmlutil/libxmlutil.la \
                     $(top_builddir)/libs/scene/libscenegraph.la \
                     $(top_builddir)/libs/math/libmath.la
mapdoom3_la_LDFLAGS = -module -avoid-version \
                      $(GTKMM_LIBS) $(XML_LIBS) $(GLEW_LIBS) $(GL_LIBS)
mapdoom3_la_SOURCES = Doom3MapFormat.cpp \
                      Doom3PrefabFormat.cpp \
                      Quake3MapFormat.cpp \
                      Quake3MapReader.cpp \
                      Quake4MapFormat.cpp \
                      Quake4MapReader.cpp \
                      Doom3MapReader.cpp \
                      mapdoom3.cpp \
                      Doom3MapWriter.cpp \
                      compiler/Doom3MapCompiler.cpp \
                      compiler/OptIsland.cpp \
                      compiler/ProcCompiler.cpp \
                      compiler/ProcFile.cpp \
                      compiler/ProcPatch.cpp \
                      compiler/ProcWinding.cpp \
                      compiler/ProcLight.cpp \
                      compiler/Surface.cpp \
                      primitiveparsers/BrushDef.cpp \
                      primitiveparsers/BrushDef3.cpp \
                      primitiveparsers/Patch.cpp \
                      primitiveparsers/PatchDef2.cpp \
                      primitiveparsers/PatchDef3.cpp

