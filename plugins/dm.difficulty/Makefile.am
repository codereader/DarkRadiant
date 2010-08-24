AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
   			  $(GTKMM_CFLAGS) $(XML_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_difficulty.la

dm_difficulty_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.la \
      						  $(top_builddir)/libs/xmlutil/libxmlutil.la
dm_difficulty_la_LDFLAGS = -module -avoid-version $(GTKMM_LIBS) $(XML_LIBS)
dm_difficulty_la_SOURCES = DifficultyEditor.cpp \
						   plugin.cpp \
						   DifficultySettingsManager.cpp \
						   Setting.cpp \
						   DifficultySettings.cpp \
						   ClassNameStore.cpp \
						   DifficultyDialog.cpp \
						   DifficultyEntity.cpp
