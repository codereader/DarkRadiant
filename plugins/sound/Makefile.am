AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = sound.la

sound_la_LIBADD = $(top_builddir)/libs/wxutil/libwxutil.la
sound_la_LDFLAGS = -module -avoid-version \
				   -lpthread \
				   $(ALUT_LIBS) $(WX_LIBS) $(VORBIS_LIBS) $(AL_LIBS)
sound_la_SOURCES = SoundManager.cpp sound.cpp SoundPlayer.cpp SoundShader.cpp

