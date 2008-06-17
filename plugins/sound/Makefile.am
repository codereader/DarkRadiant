AM_CPPFLAGS = -I../../include -I../../libs $(GTK_CFLAGS)

pkglib_LTLIBRARIES = sound.la
sound_la_LDFLAGS = -module -avoid-version
sound_la_SOURCES = SoundManager.cpp sound.cpp SoundPlayer.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/sound.so "$(DESTDIR)$(pkglibdir)/modules"

