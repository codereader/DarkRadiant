libspath = ../../libs
AM_CPPFLAGS = -I../../include -I$(libspath) $(GTK_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = sound.la

sound_la_LIBADD = $(libspath)/gtkutil/libgtkutil.a
sound_la_LDFLAGS = -module -avoid-version \
				   $(ALUT_LIBS) $(GTK_LIBS) $(VORBIS_LIBS) $(AL_LIBS)
sound_la_SOURCES = SoundManager.cpp sound.cpp SoundPlayer.cpp

