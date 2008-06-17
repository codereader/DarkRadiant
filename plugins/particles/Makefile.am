AM_CPPFLAGS = -I../../include -I../../libs 

pkglib_LTLIBRARIES = particles.la
particles_la_LDFLAGS = -module -avoid-version
particles_la_SOURCES = ParticlesManager.cpp particles.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/particles.so "$(DESTDIR)$(pkglibdir)/modules"

