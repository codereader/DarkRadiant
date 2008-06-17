AM_CPPFLAGS = -I../../include -I../../libs $(XML_CFLAGS)

pkglib_LTLIBRARIES = eclassmgr.la
eclassmgr_la_LDFLAGS = -module -avoid-version
eclassmgr_la_SOURCES = Doom3EntityClass.cpp EClassManager.cpp eclass_doom3.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/eclassmgr.so "$(DESTDIR)$(pkglibdir)/modules"

