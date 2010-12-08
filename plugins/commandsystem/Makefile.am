AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
			  $(XML_CFLAGS) $(LIBSIGC_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = commandsystem.la

commandsystem_la_LDFLAGS = -module -avoid-version 
commandsystem_la_LIBADD = $(top_builddir)/libs/xmlutil/libxmlutil.la \
                          $(XML_LIBS) \
                          $(LIBSIGC_LIBS)
commandsystem_la_SOURCES = CommandSystem.cpp
