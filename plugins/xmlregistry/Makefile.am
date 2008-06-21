libspath = ../../libs
AM_CPPFLAGS = -DPKGDATADIR='"$(pkgdatadir)"' \
			  -I../../include -I$(libspath) \
			  $(XML_CFLAGS) $(GLIB_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = xmlregistry.la

xmlregistry_la_LIBADD = $(libspath)/xmlutil/libxmlutil.a
xmlregistry_la_LDFLAGS = -module -avoid-version $(XML_LIBS) $(GLIB_LIBS)
xmlregistry_la_SOURCES = RegistryTree.cpp XMLRegistry.cpp XMLRegistryModule.cpp

