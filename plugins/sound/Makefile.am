AM_CPPFLAGS = -I../../include -I../../libs $(GTK_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = sound.la
sound_la_LDFLAGS = -module -avoid-version $(ALUT_LIBS)
sound_la_SOURCES = SoundManager.cpp sound.cpp SoundPlayer.cpp

