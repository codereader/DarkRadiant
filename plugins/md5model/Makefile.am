AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(LIBSIGC_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = md5model.la

md5model_la_LIBADD = $(top_builddir)/libs/scene/libscenegraph.la \
					 $(top_builddir)/libs/math/libmath.la
md5model_la_LDFLAGS = -module -avoid-version \
                      $(GLEW_LIBS) $(GL_LIBS) $(LIBSIGC_LIBS)
md5model_la_SOURCES = MD5Model.cpp \
                      MD5ModelNode.cpp \
                      MD5Surface.cpp \
                      plugin.cpp \
                      MD5ModelLoader.cpp \
					  MD5Skeleton.cpp \
					  MD5AnimationCache.cpp \
					  MD5Anim.cpp

