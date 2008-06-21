libspath = ../../libs
AM_CPPFLAGS = -I../../include -I$(libspath) $(XML_CFLAGS) $(GTK_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = uimanager.la

uimanager_la_LDFLAGS = -module -avoid-version $(GTK_LIBS) $(XML_LIBS)
uimanager_la_LIBADD = $(libspath)/gtkutil/libgtkutil.a \
					  $(libspath)/xmlutil/libxmlutil.a
uimanager_la_SOURCES = MenuItem.cpp \
                       GroupDialog.cpp \
                       colourscheme/ColourScheme.cpp \
                       colourscheme/ColourSchemeEditor.cpp \
                       colourscheme/ColourSchemeManager.cpp \
                       ToolbarManager.cpp \
                       UIManager.cpp \
                       MenuManager.cpp

