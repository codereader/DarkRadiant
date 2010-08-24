AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(GTKMM_CFLAGS) $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = eventmgr.la

eventmgr_la_LIBADD = $(top_builddir)/libs/xmlutil/libxmlutil.la
eventmgr_la_LDFLAGS = -module -avoid-version \
                      $(GTKMM_LIBS) $(GLIB_LIBS) $(XML_LIBS)
eventmgr_la_SOURCES = Accelerator.cpp \
                      Statement.cpp \
                      EventManager.cpp \
                      Modifiers.cpp \
                      MouseEvents.cpp \
                      Toggle.cpp \
                      WidgetToggle.cpp

