AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
               $(XML_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_editing.la

dm_editing_la_LIBADD = $(top_builddir)/libs/wxutil/libwxutil.la \
                    $(top_builddir)/libs/xmlutil/libxmlutil.la
dm_editing_la_LDFLAGS = -module -avoid-version \
                        $(WX_LIBS) \
                        $(XML_LIBS) \
                        $(FILESYSTEM_LIBS)
dm_editing_la_SOURCES = plugin.cpp \
                  AIHeadPropertyEditor.cpp \
				  AIEditingPanel.cpp \
                  FixupMap.cpp \
                  DarkmodTxt.cpp \
                  ReadmeTxt.cpp \
                  MissionInfoTextFile.cpp \
				  MissionInfoEditDialog.cpp \
				  MissionReadmeDialog.cpp \
                  MissionInfoGuiView.cpp \
                  FixupMapDialog.cpp \
                  AIVocalSetChooserDialog.cpp \
                  AIVocalSetPropertyEditor.cpp \
                  AIVocalSetPreview.cpp \
                  AIHeadChooserDialog.cpp
