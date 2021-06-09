add_library(dm_objectives MODULE
            ce/AIFindBodyComponentEditor.cpp
            ce/AIFindItemComponentEditor.cpp
            ce/AlertComponentEditor.cpp
            ce/ComponentEditorFactory.cpp
            ce/CustomClockedComponentEditor.cpp
            ce/CustomComponentEditor.cpp
            ce/DestroyComponentEditor.cpp
            ce/DistanceComponentEditor.cpp
            ce/InfoLocationComponentEditor.cpp
            ce/ItemComponentEditor.cpp
            ce/KillComponentEditor.cpp
            ce/KnockoutComponentEditor.cpp
            ce/LocationComponentEditor.cpp
            ce/PickpocketComponentEditor.cpp
            ce/ReadableClosedComponentEditor.cpp
            ce/ReadableOpenedComponentEditor.cpp
            ce/ReadablePageReachedComponentEditor.cpp
            ce/SpecifierEditCombo.cpp
            ce/specpanel/AIInnocenceSpecifierPanel.cpp
            ce/specpanel/AITeamSpecifierPanel.cpp
            ce/specpanel/AITypeSpecifierPanel.cpp
            ce/specpanel/ClassnameSpecifierPanel.cpp
            ce/specpanel/EntityNameSpecifierPanel.cpp
            ce/specpanel/GroupSpecifierPanel.cpp
            ce/specpanel/SpawnClassSpecifierPanel.cpp
            ce/specpanel/SpecifierPanelFactory.cpp
            ce/specpanel/TextSpecifierPanel.cpp
            Component.cpp
            ComponentsDialog.cpp
            ComponentType.cpp
            DifficultyPanel.cpp
            LogicEditor.cpp
            MissionLogicDialog.cpp
            ObjectiveConditionsDialog.cpp
            ObjectiveEntity.cpp
            ObjectiveEntityFinder.cpp
            ObjectiveKeyExtractor.cpp
            objectives.cpp
            ObjectivesEditor.cpp
            Specifier.cpp
            SpecifierType.cpp)
target_compile_options(dm_objectives PUBLIC ${SIGC_CFLAGS})
target_link_libraries(dm_objectives PUBLIC wxutil)

# Enable precompiled header for dm_objectives
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
target_precompile_headers(dm_objectives PRIVATE "precompiled.h")
endif()