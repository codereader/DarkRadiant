AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(GTKMM_CFLAGS) $(XML_CFLAGS) $(GTKGLEXTMM_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_gui.la

dm_gui_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.la \
                   $(top_builddir)/libs/xmlutil/libxmlutil.la
dm_gui_la_LDFLAGS = -module -avoid-version \
                    $(GTKMM_LIBS) \
                    $(GTKGLEXTMM_LIBS) \
                    $(XML_LIBS) \
                    $(GLEW_LIBS) \
                    $(GL_LIBS) \
                    $(BOOST_SYSTEM_LIBS) \
                    $(BOOST_FILESYSTEM_LIBS)
dm_gui_la_SOURCES = GuiSelector.cpp \
               plugin.cpp \
               ReadableEditorDialog.cpp \
               ReadableGuiView.cpp \
               XData.cpp \
               XDataLoader.cpp \
               XDataSelector.cpp \
               XdFileChooserDialog.cpp \
               gui/Gui.cpp \
               gui/GuiManager.cpp \
               gui/GuiRenderer.cpp \
               gui/GuiScript.cpp \
               gui/GuiView.cpp \
               gui/GuiWindowDef.cpp \
               gui/RenderableCharacterBatch.cpp \
               gui/RenderableText.cpp \
               gui/Variable.cpp
