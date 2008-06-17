AM_CPPFLAGS = -I../../include -I../../libs $(XML_CFLAGS) $(GTK_CFLAGS)

pkglib_LTLIBRARIES = mapdoom3.la
mapdoom3_la_LDFLAGS = -module -avoid-version
mapdoom3_la_SOURCES = Doom3MapFormat.cpp \
                      NodeImporter.cpp \
                      InfoFile.cpp \
                      mapdoom3.cpp \
                      NodeExporter.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/mapdoom3.so "$(DESTDIR)$(pkglibdir)/modules"

