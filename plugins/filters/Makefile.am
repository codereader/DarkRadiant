AM_CPPFLAGS = -I../../include -I../../libs $(XML_CFLAGS)

pkglib_LTLIBRARIES = filters.la
filters_la_LDFLAGS = -module -avoid-version
filters_la_SOURCES = XMLFilter.cpp BasicFilterSystem.cpp filters.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/filters.so "$(DESTDIR)$(pkglibdir)/modules"

