AM_CPPFLAGS = -I../../include -I../../libs $(XML_CFLAGS) $(GTK_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_objectives.la
dm_objectives_la_LDFLAGS = -module -avoid-version
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
