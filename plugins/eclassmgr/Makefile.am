AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = eclassmgr.la

eclassmgr_la_LIBADD = ../../libs/math/libmath.a
eclassmgr_la_LDFLAGS = -module -avoid-version
eclassmgr_la_SOURCES = Doom3EntityClass.cpp EClassManager.cpp eclass_doom3.cpp

