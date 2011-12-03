AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(LIBSIGC_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = model.la

model_la_LDFLAGS = -module -avoid-version \
                   $(GLEW_LIBS) $(GL_LIBS) $(LIBSIGC_LIBS)
model_la_LIBADD = $(top_builddir)/libs/picomodel/libpicomodel.la \
				  $(top_builddir)/libs/math/libmath.la \
				  $(top_builddir)/libs/scene/libscenegraph.la
model_la_SOURCES = PicoModelNode.cpp \
                   RenderablePicoModel.cpp \
                   PicoModelLoader.cpp \
                   RenderablePicoSurface.cpp \
                   plugin.cpp

