AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = shaders.la

shaders_la_LIBADD = $(top_builddir)/libs/xmlutil/libxmlutil.la
shaders_la_LDFLAGS = -module -avoid-version $(XML_LIBS) $(GL_LIBS) $(GLU_LIBS)
shaders_la_SOURCES = ShaderTemplate.cpp \
                     CameraCubeMapDecl.cpp \
                     CShader.cpp \
                     ShaderLibrary.cpp \
                     MapExpression.cpp \
                     ShaderFileLoader.cpp \
                     plugin.cpp \
                     textures/TextureManipulator.cpp \
                     textures/FileLoader.cpp \
                     textures/ImageLoaderManager.cpp \
                     textures/GLTextureManager.cpp \
                     Doom3ShaderSystem.cpp \
					 Doom3ShaderLayer.cpp

