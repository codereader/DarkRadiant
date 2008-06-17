AM_CPPFLAGS = -I../../include -I../../libs $(XML_CFLAGS)

pkglib_LTLIBRARIES = undo.la
undo_la_LDFLAGS = -module -avoid-version
undo_la_SOURCES = UndoSystem.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/undo.so "$(DESTDIR)$(pkglibdir)/modules"

