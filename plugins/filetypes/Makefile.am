AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
               $(LIBSIGC_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = filetypes.la

filetypes_la_LDFLAGS = -module -avoid-version $(WX_LIBS) $(LIBSIGC_LIBS)
filetypes_la_SOURCES = FileTypeRegistry.cpp

