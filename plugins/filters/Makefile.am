AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = filters.la

filters_la_LIBADD = $(top_builddir)/libs/xmlutil/libxmlutil.la
filters_la_LDFLAGS = -module -avoid-version $(REGEX_LIBS) $(XML_LIBS)
filters_la_SOURCES = XMLFilter.cpp BasicFilterSystem.cpp filters.cpp

