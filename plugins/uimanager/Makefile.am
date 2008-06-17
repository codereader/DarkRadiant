AM_CPPFLAGS = -I../../include -I../../libs $(XML_CFLAGS) $(GTK_CFLAGS)

pkglib_LTLIBRARIES = uimanager.la
uimanager_la_LDFLAGS = -module -avoid-version
uimanager_la_SOURCES = MenuItem.cpp \
                       GroupDialog.cpp \
                       colourscheme/ColourScheme.cpp \
                       colourscheme/ColourSchemeEditor.cpp \
                       colourscheme/ColourSchemeManager.cpp \
                       ToolbarManager.cpp \
                       UIManager.cpp \
                       MenuManager.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/uimanager.so "$(DESTDIR)$(pkglibdir)/modules"

