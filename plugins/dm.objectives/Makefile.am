AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
               $(XML_CFLAGS) $(GTKMM_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_objectives.la

dm_objectives_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.la \
                          $(top_builddir)/libs/xmlutil/libxmlutil.la
dm_objectives_la_LDFLAGS = -module -avoid-version -Wl,-z,defs \
                           $(BOOST_REGEX_LIBS) $(GTKMM_LIBS) $(XML_LIBS)
dm_objectives_la_SOURCES = ObjectivesEditor.cpp \
                     objectives.cpp \
                     SpecifierType.cpp \
                     ce/ComponentEditorFactory.cpp \
                     ce/KnockoutComponentEditor.cpp \
                     ce/KillComponentEditor.cpp \
                     ce/SpecifierEditCombo.cpp \
                     ce/LocationComponentEditor.cpp \
                     ce/AIFindBodyComponentEditor.cpp \
                     ce/AlertComponentEditor.cpp \
                     ce/ItemComponentEditor.cpp \
                     ce/InfoLocationComponentEditor.cpp \
                     ce/CustomComponentEditor.cpp \
                     ce/DistanceComponentEditor.cpp \
                     ce/CustomClockedComponentEditor.cpp \
                     ce/PickpocketComponentEditor.cpp \
                     ce/DestroyComponentEditor.cpp \
                     ce/AIFindItemComponentEditor.cpp \
                     ce/ReadableOpenedComponentEditor.cpp \
                     ce/ReadableClosedComponentEditor.cpp \
                     ce/ReadablePageReachedComponentEditor.cpp \
                     ce/specpanel/AIInnocenceSpecifierPanel.cpp \
                     ce/specpanel/AITypeSpecifierPanel.cpp \
                     ce/specpanel/SpawnClassSpecifierPanel.cpp \
                     ce/specpanel/ClassnameSpecifierPanel.cpp \
                     ce/specpanel/AITeamSpecifierPanel.cpp \
                     ce/specpanel/GroupSpecifierPanel.cpp \
                     ce/specpanel/SpecifierPanelFactory.cpp \
                     ce/specpanel/EntityNameSpecifierPanel.cpp \
                     ce/specpanel/TextSpecifierPanel.cpp \
                     ObjectiveConditionsDialog.cpp \
                     ObjectiveEntity.cpp \
                     ObjectiveKeyExtractor.cpp \
                     ObjectiveEntityFinder.cpp \
                     Component.cpp \
                     ComponentsDialog.cpp \
                     ComponentType.cpp \
                     DifficultyPanel.cpp \
                     LogicEditor.cpp \
                     MissionLogicDialog.cpp \
                     Specifier.cpp
