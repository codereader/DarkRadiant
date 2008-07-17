AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
			  $(XML_CFLAGS) $(GTK_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_objectives.la

dm_objectives_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.a
dm_objectives_la_LDFLAGS = -module -avoid-version $(REGEX_LIBS) $(GTK_LIBS)
dm_objectives_la_SOURCES = ObjectivesEditor.cpp \
						   objectives.cpp \
						   SpecifierType.cpp \
						   ce/ComponentEditorFactory.cpp \
						   ce/KnockoutComponentEditor.cpp \
						   ce/KillComponentEditor.cpp \
						   ce/SpecifierEditCombo.cpp \
						   ce/specpanel/SpecifierPanelFactory.cpp \
						   ce/specpanel/EntityNameSpecifierPanel.cpp \
						   ce/specpanel/TextSpecifierPanel.cpp \
						   ObjectiveEntity.cpp \
						   ObjectiveKeyExtractor.cpp \
						   ComponentsDialog.cpp \
						   ComponentType.cpp
