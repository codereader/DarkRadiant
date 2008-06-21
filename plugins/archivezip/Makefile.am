AM_CPPFLAGS = -I../../include -I../../libs

pkglib_LTLIBRARIES = archivezip.la
archivezip_la_LDFLAGS = -module -avoid-version $(Z_LIBS)
archivezip_la_SOURCES = ZipArchive.cpp pkzip.cpp plugin.cpp zlibstream.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/archivezip.so "$(DESTDIR)$(pkglibdir)/modules"

