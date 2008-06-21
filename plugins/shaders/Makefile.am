libspath = ../../libs
AM_CPPFLAGS = -I../../include -I$(libspath) $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = shaders.la

shaders_la_LIBADD = $(libspath)/xmlutil/libxmlutil.a
shaders_la_LDFLAGS = -module -avoid-version $(XML_LIBS) $(GL_LIBS) $(GLU_LIBS)
shaders_la_SOURCES = ShaderTemplate.cpp \
                     CShader.cpp \
                     ShaderLibrary.cpp \
                     MapExpression.cpp \
                     ShaderFileLoader.cpp \
                     plugin.cpp \
                     textures/TextureManipulator.cpp \
                     textures/FileLoader.cpp \
                     textures/DefaultConstructor.cpp \
                     textures/ImageLoaderManager.cpp \
                     textures/GLTextureManager.cpp \
                     Doom3ShaderSystem.cpp

