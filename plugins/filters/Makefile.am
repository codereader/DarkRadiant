AM_CPPFLAGS = -I../../include -I../../libs $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = filters.la
filters_la_LDFLAGS = -module -avoid-version $(REGEX_LIBS)
filters_la_SOURCES = XMLFilter.cpp BasicFilterSystem.cpp filters.cpp

