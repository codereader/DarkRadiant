AM_CPPFLAGS = -I../../include -I../../libs $(XML_CFLAGS)

pkglib_LTLIBRARIES = shaders.la
shaders_la_LDFLAGS = -module -avoid-version
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

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/shaders.so "$(DESTDIR)$(pkglibdir)/modules"

