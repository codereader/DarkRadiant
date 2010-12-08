AM_CPPFLAGS = -DPKGDATADIR='"$(pkgdatadir)"' \
			  -I$(top_srcdir)/include -I$(top_srcdir)/libs \
			  $(XML_CFLAGS) $(GLIB_CFLAGS) $(LIBSIGC_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = xmlregistry.la

xmlregistry_la_LIBADD = $(top_builddir)/libs/xmlutil/libxmlutil.la
xmlregistry_la_LDFLAGS = -module -avoid-version \
                         $(XML_LIBS) $(GLIB_LIBS) $(LIBSIGC_LIBS)
xmlregistry_la_SOURCES = RegistryTree.cpp XMLRegistry.cpp XMLRegistryModule.cpp

