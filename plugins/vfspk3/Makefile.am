AM_CPPFLAGS = -I../../include -I../../libs $(GLIB_CFLAGS)

pkglib_LTLIBRARIES = vfspk3.la
vfspk3_la_LDFLAGS = -module -avoid-version
vfspk3_la_SOURCES = vfspk3.cpp Doom3FileSystem.cpp DirectoryArchive.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/vfspk3.so "$(DESTDIR)$(pkglibdir)/modules"

