AM_CPPFLAGS = -I../../include -I../../libs $(XML_CFLAGS)

pkglib_LTLIBRARIES = grid.la
grid_la_LDFLAGS = -module -avoid-version
grid_la_SOURCES = Grid.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/grid.so "$(DESTDIR)$(pkglibdir)/modules"

