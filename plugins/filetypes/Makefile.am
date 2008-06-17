AM_CPPFLAGS = -I../../include -I../../libs

pkglib_LTLIBRARIES = filetypes.la
filetypes_la_LDFLAGS = -module -avoid-version
filetypes_la_SOURCES = FileTypeRegistry.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/filetypes.so "$(DESTDIR)$(pkglibdir)/modules"

