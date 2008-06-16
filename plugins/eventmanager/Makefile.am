AM_CPPFLAGS = -I../../include -I../../libs \
              $(GTK_CFLAGS) $(XML_CFLAGS)

pkglib_LTLIBRARIES = eventmgr.la
eventmgr_la_LDFLAGS = -module -shared -avoid-version
eventmgr_la_SOURCES = Accelerator.cpp \
                      Command.cpp \
                      EventManager.cpp \
                      Modifiers.cpp \
                      MouseEvents.cpp \
                      Toggle.cpp \
                      WidgetToggle.cpp
