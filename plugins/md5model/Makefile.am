libspath = ../../libs
AM_CPPFLAGS = -I../../include -I$(libspath)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = md5model.la

md5model_la_LIBADD = $(libspath)/scene/libscenegraph.a \
					 $(libspath)/math/libmath.a
md5model_la_LDFLAGS = -module -avoid-version $(GLEW_LIBS) $(GL_LIBS)
md5model_la_SOURCES = MD5Model.cpp \
                      MD5ModelNode.cpp \
                      MD5Surface.cpp \
                      plugin.cpp \
                      MD5ModelLoader.cpp

