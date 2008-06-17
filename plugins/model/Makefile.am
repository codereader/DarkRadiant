AM_CPPFLAGS = -I../../include -I../../libs 

pkglib_LTLIBRARIES = model.la
model_la_LDFLAGS = -module -avoid-version
model_la_SOURCES = PicoModelNode.cpp \
                   RenderablePicoModel.cpp \
                   PicoModelLoader.cpp \
                   RenderablePicoSurface.cpp \
                   plugin.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/model.so "$(DESTDIR)$(pkglibdir)/modules"

