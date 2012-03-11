AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
               $(XML_CFLAGS) $(GTKMM_CFLAGS) $(GTKGLEXTMM_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = uimanager.la

uimanager_la_LDFLAGS = -module -avoid-version \
                       $(GTKMM_LIBS) \
                       $(XML_LIBS) \
                       $(GTKGLEXTMM_LIBS) \
                       $(GLEW_LIBS) \
                       $(GL_LIBS)
uimanager_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.la \
                      $(top_builddir)/libs/math/libmath.la \
                      $(top_builddir)/libs/xmlutil/libxmlutil.la
uimanager_la_SOURCES = MenuItem.cpp \
                       GroupDialog.cpp \
                       colourscheme/ColourScheme.cpp \
                       colourscheme/ColourSchemeEditor.cpp \
                       colourscheme/ColourSchemeManager.cpp \
                       StatusBarManager.cpp \
                       DialogManager.cpp \
                       ToolbarManager.cpp \
                       UIManager.cpp \
                       FilterMenu.cpp \
                       MenuManager.cpp
