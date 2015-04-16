AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(XML_CFLAGS)

pkglib_LTLIBRARIES = libxmlutil.la
libxmlutil_la_LDFLAGS = -release @PACKAGE_VERSION@ $(XML_LIBS)
libxmlutil_la_SOURCES = Document.cpp Node.cpp
