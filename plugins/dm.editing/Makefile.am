AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
               $(GTKMM_CFLAGS) $(XML_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_editing.la

dm_editing_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.la \
                    $(top_builddir)/libs/xmlutil/libxmlutil.la
dm_editing_la_LDFLAGS = -module -avoid-version \
                        $(GTKMM_LIBS) $(XML_LIBS) $(BOOST_REGEX_LIBS)
dm_editing_la_SOURCES = plugin.cpp \
                  AIHeadPropertyEditor.cpp \
				  AIEditingPanel.cpp \
                  FixupMap.cpp \
                  FixupMapDialog.cpp \
                  AIVocalSetChooserDialog.cpp \
                  AIVocalSetPropertyEditor.cpp \
                  AIVocalSetPreview.cpp \
                  AIHeadChooserDialog.cpp
