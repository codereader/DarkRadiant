libspath = ../../libs
AM_CPPFLAGS = -I../../include -I$(libspath)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = model.la

model_la_LDFLAGS = -module -avoid-version $(GLEW_LIBS) $(GL_LIBS)
model_la_LIBADD = $(libspath)/picomodel/libpicomodel.a \
				  $(libspath)/math/libmath.a \
				  $(libspath)/scene/libscenegraph.a
model_la_SOURCES = PicoModelNode.cpp \
                   RenderablePicoModel.cpp \
                   PicoModelLoader.cpp \
                   RenderablePicoSurface.cpp \
                   plugin.cpp

