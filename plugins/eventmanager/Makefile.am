AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
		$(LIBSIGC_CFLAGS) \
		$(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = eventmgr.la

eventmgr_la_LIBADD = $(top_builddir)/libs/xmlutil/libxmlutil.la \
                     $(top_builddir)/libs/wxutil/libwxutil.la
eventmgr_la_LDFLAGS = -module -avoid-version \
                      $(LIBSIGC_LIBS) $(XML_LIBS) $(WX_LIBS) $(BOOST_REGEX_LIBS)
eventmgr_la_SOURCES = Accelerator.cpp \
                      Statement.cpp \
                      EventManager.cpp \
                      GlobalKeyEventFilter.cpp \
                      MouseToolGroup.cpp \
                      MouseToolManager.cpp \
                      Toggle.cpp \
                      WidgetToggle.cpp

