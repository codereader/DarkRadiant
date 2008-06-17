AM_CPPFLAGS = -I../../include -I../../libs \
              $(GTK_CFLAGS) $(XML_CFLAGS)

pkglib_LTLIBRARIES = eventmgr.la
eventmgr_la_LDFLAGS = -module -avoid-version
eventmgr_la_SOURCES = Accelerator.cpp \
                      Command.cpp \
                      EventManager.cpp \
                      Modifiers.cpp \
                      MouseEvents.cpp \
                      Toggle.cpp \
                      WidgetToggle.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/eventmgr.so "$(DESTDIR)$(pkglibdir)/modules"

