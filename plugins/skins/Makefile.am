AM_CPPFLAGS = -I../../include -I../../libs 

pkglib_LTLIBRARIES = skins.la
skins_la_LDFLAGS = -module -avoid-version
skins_la_SOURCES = Doom3SkinCache.cpp skincache.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/skins.so "$(DESTDIR)$(pkglibdir)/modules"

