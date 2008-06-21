libspath = ../../libs
AM_CPPFLAGS = -I../../include -I$(libspath) $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = filters.la

filters_la_LIBADD = $(libspath)/xmlutil/libxmlutil.a
filters_la_LDFLAGS = -module -avoid-version $(REGEX_LIBS) $(XML_LIBS)
filters_la_SOURCES = XMLFilter.cpp BasicFilterSystem.cpp filters.cpp

