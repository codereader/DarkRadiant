AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
               $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = uimanager.la

uimanager_la_LDFLAGS = -module -avoid-version \
                       $(WX_LIBS) \
                       $(XML_LIBS) \
                       $(BOOST_FILESYSTEM_LIBS) \
                       $(BOOST_SYSTEM_LIBS) \
                       $(GLEW_LIBS) \
                       $(GL_LIBS)
uimanager_la_LIBADD = $(top_builddir)/libs/wxutil/libwxutil.la \
                      $(top_builddir)/libs/math/libmath.la \
                      $(top_builddir)/libs/scene/libscenegraph.la \
                      $(top_builddir)/libs/xmlutil/libxmlutil.la
uimanager_la_SOURCES = MenuItem.cpp \
                       GroupDialog.cpp \
					   animationpreview/MD5AnimationChooser.cpp \
					   animationpreview/MD5AnimationViewer.cpp \
					   animationpreview/AnimationPreview.cpp \
                       colourscheme/ColourScheme.cpp \
                       colourscheme/ColourSchemeEditor.cpp \
                       colourscheme/ColourSchemeManager.cpp \
					   SoundChooser.cpp \
					   SoundShaderPreview.cpp \
                       StatusBarManager.cpp \
                       DialogManager.cpp \
                       ToolbarManager.cpp \
                       UIManager.cpp \
                       FilterMenu.cpp \
                       MenuManager.cpp
