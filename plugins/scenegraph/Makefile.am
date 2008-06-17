AM_CPPFLAGS = -I../../include -I../../libs 

pkglib_LTLIBRARIES = scenegraph.la
scenegraph_la_LDFLAGS = -module -avoid-version
scenegraph_la_SOURCES = SceneGraph.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/scenegraph.so "$(DESTDIR)$(pkglibdir)/modules"

