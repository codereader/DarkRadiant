AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(LIBSIGC_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = skins.la

skins_la_LDFLAGS = -module -avoid-version $(LIBSIGC_LIBS)
skins_la_SOURCES = Doom3SkinCache.cpp skincache.cpp

