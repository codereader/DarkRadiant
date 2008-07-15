AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
			  $(GTK_CFLAGS) $(XML_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_difficulty.la

dm_difficulty_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.a \
						  $(top_builddir)/libs/xmlutil/libxmlutil.a
dm_difficulty_la_LDFLAGS = -module -avoid-version $(GTK_LIBS) $(XML_LIBS)
dm_difficulty_la_SOURCES = DifficultyEditor.cpp \
						   plugin.cpp \
						   DifficultySettingsManager.cpp \
						   Setting.cpp \
						   DifficultySettings.cpp \
						   ClassNameStore.cpp \
						   DifficultyDialog.cpp \
						   DifficultyEntity.cpp
