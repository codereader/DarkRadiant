AM_CPPFLAGS = -DPKGDATADIR='"$(pkgdatadir)"' \
			  -I../../include -I../../libs \
			  $(XML_CFLAGS) $(GLIB_CFLAGS)

pkglib_LTLIBRARIES = xmlregistry.la
xmlregistry_la_LDFLAGS = -module -avoid-version
xmlregistry_la_SOURCES = RegistryTree.cpp XMLRegistry.cpp XMLRegistryModule.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/xmlregistry.so "$(DESTDIR)$(pkglibdir)/modules"

