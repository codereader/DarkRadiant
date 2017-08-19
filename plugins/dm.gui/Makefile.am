AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(XML_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_gui.la

dm_gui_la_LIBADD = $(top_builddir)/libs/wxutil/libwxutil.la \
                   $(top_builddir)/libs/xmlutil/libxmlutil.la
dm_gui_la_LDFLAGS = -module -avoid-version \
					-lpthread \
                    $(WX_LIBS) \
                    $(XML_LIBS) \
                    $(GLEW_LIBS) \
                    $(GL_LIBS) \
                    $(FILESYSTEM_LIBS)
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
