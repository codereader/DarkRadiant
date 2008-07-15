AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(GTK_CFLAGS) $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = eventmgr.la

eventmgr_la_LIBADD = ../../libs/xmlutil/libxmlutil.a
eventmgr_la_LDFLAGS = -module -avoid-version \
					  $(GTK_LIBS) $(GLIB_LIBS) $(XML_LIBS)
eventmgr_la_SOURCES = Accelerator.cpp \
                      Command.cpp \
                      EventManager.cpp \
                      Modifiers.cpp \
                      MouseEvents.cpp \
                      Toggle.cpp \
                      WidgetToggle.cpp

