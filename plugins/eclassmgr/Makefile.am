AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(GTKMM_CFLAGS) $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = eclassmgr.la

eclassmgr_la_LIBADD = $(top_builddir)/libs/math/libmath.la
eclassmgr_la_LDFLAGS = -module -avoid-version $(GTKMM_LIBS)
eclassmgr_la_SOURCES = Doom3EntityClass.cpp EClassManager.cpp eclass_doom3.cpp

