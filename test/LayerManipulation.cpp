#include "i18n.h"
#include "RadiantTest.h"

#include "imap.h"
#include "ilayer.h"
#include "scenelib.h"
#include "os/file.h"

#include "algorithm/Scene.h"
#include "testutil/FileSaveConfirmationHelper.h"
#include "testutil/TemporaryFile.h"

namespace test
{

using LayerTest = RadiantTest;

inline std::vector<std::string> getAllLayerNames()
{
    std::vector<std::string> layerNames;

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    layerManager.foreachLayer([&](int layerId, const std::string& layerName)
    {
        layerNames.push_back(layerName);
    });

    return layerNames;
}

inline std::vector<int> getAllLayerIds()
{
    std::vector<int> layerIds;

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    layerManager.foreachLayer([&](int layerId, const std::string& layerName)
    {
        layerIds.push_back(layerId);
    });

    return layerIds;
}

TEST_F(LayerTest, CreateLayer)
{
    EXPECT_EQ(getAllLayerNames().size(), 1) << "Expected a default layer at map start";
    EXPECT_EQ(getAllLayerNames(), std::vector{ _("Default") }) << "Expected a default layer at map start";

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    const std::string testLayerName = "TestLayer";

    std::size_t layersChangedFireCount = 0;
    layerManager.signal_layersChanged().connect([&]()
    {
        ++layersChangedFireCount;
    });

    auto layerId =layerManager.createLayer(testLayerName);

    EXPECT_GT(layerId, 0) << "New layer must not have and ID of -1 or 0";
    EXPECT_EQ(getAllLayerNames(), std::vector({ _("Default"), testLayerName })) << "Expected two layers after creating a new one";
    EXPECT_EQ(layersChangedFireCount, 1) << "Layers changed signal should have been fired";
    EXPECT_TRUE(layerManager.layerExists(layerId)) << "The test layer should exist now";
}

TEST_F(LayerTest, CreateLayerWithId)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    const std::string testLayerName = "TestLayer";
    constexpr int testLayerId = 56;

    std::size_t layersChangedFireCount = 0;
    layerManager.signal_layersChanged().connect([&]()
    {
        ++layersChangedFireCount;
    });

    auto layerId = layerManager.createLayer(testLayerName, testLayerId);

    EXPECT_EQ(layerId, testLayerId) << "New layer needs to have the ID " << testLayerId;
    EXPECT_EQ(getAllLayerNames(), std::vector({ _("Default"), testLayerName })) << "Expected two layers after creating a new one";

    EXPECT_EQ(layerManager.getLayerID(testLayerName), testLayerId) << "getLayerID reported the wrong ID";
    EXPECT_EQ(layerManager.getLayerName(testLayerId), testLayerName) << "getLayerName reported the wrong Name";
    EXPECT_EQ(layersChangedFireCount, 1) << "Layers changed signal should have been fired";
    EXPECT_TRUE(layerManager.layerExists(layerId)) << "The test layer should exist now";
}

TEST_F(LayerTest, GetLayerId)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    EXPECT_NE(layerManager.getLayerID(_("Default")), -1) << "Default layer should return an ID";

    layerManager.createLayer("TestLayer");

    EXPECT_NE(layerManager.getLayerID("TestLayer"), -1) << "Layer exists, we should get an ID";
    EXPECT_EQ(layerManager.getLayerID("nonexistent_layer"), -1) << "Layer doesn't exist, yet we got an ID";
}

TEST_F(LayerTest, LayerExists)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    EXPECT_TRUE(layerManager.layerExists(0)) << "The default layer should always exist";
    EXPECT_FALSE(layerManager.layerExists(-1)) << "Layer ID -1 should never exist";

    EXPECT_FALSE(layerManager.layerExists(1)) << "Layer ID 1 should not exist";
    EXPECT_FALSE(layerManager.layerExists(540)) << "Layer ID 540 should not exist";
}

TEST_F(LayerTest, DefaultLayer)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    EXPECT_EQ(layerManager.getLayerID(_("Default")), 0) << "Default layer should have the ID 0";
    EXPECT_EQ(layerManager.getLayerName(0), _("Default")) << "Default layer should be named "<< _("Default");
    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "Default layer should be active";
    EXPECT_EQ(layerManager.layerIsVisible(0), true) << "Default layer should be visible";
}

TEST_F(LayerTest, DeleteDefaultLayer)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto defaultLayerName = layerManager.getLayerName(0);

    EXPECT_EQ(getAllLayerNames(), std::vector({ defaultLayerName })) << "Expected one default layer";

    std::size_t layersChangedFireCount = 0;
    layerManager.signal_layersChanged().connect([&]()
    {
        ++layersChangedFireCount;
    });

    // Attempting to delete the default layer doesn't work
    layerManager.deleteLayer(defaultLayerName);

    EXPECT_TRUE(layerManager.layerExists(0)) << "The default layer should still exist";
    EXPECT_EQ(getAllLayerNames(), std::vector({ defaultLayerName })) << "Expected still one default layer";
    EXPECT_EQ(layersChangedFireCount, 0) << "Layers changed signal should NOT have been fired";
}

TEST_F(LayerTest, DeleteLayer)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    const std::string testLayerName = "TestLayer";
    auto layerId = layerManager.createLayer(testLayerName);

    EXPECT_TRUE(layerManager.layerExists(layerId)) << "The test layer should have been created";
    EXPECT_EQ(layerManager.getLayerID(testLayerName), layerId) << "getLayerId reports a different ID";

    std::size_t layersChangedFireCount = 0;
    layerManager.signal_layersChanged().connect([&]()
    {
        ++layersChangedFireCount;
    });

    // Attempting to delete the default layer doesn't work
    layerManager.deleteLayer(testLayerName);

    EXPECT_EQ(layerManager.getLayerID(testLayerName), -1) << "The test layer should be gone now";
    EXPECT_EQ(layersChangedFireCount, 1) << "Layers changed signal should have been fired";
    EXPECT_FALSE(layerManager.layerExists(layerId)) << "The test layer should be gone now";
}

TEST_F(LayerTest, ResetLayerManager)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    auto testLayerId = layerManager.createLayer("TestLayer");
    auto testLayer2Id = layerManager.createLayer("TestLayer2");

    // Test Layer 2 is a child of Test Layer
    layerManager.setParentLayer(testLayer2Id, testLayerId);

    std::size_t layersChangedFireCount = 0;
    std::size_t layerVisibilityChangedFireCount = 0;
    std::size_t layerHierarchyChangedFireCount = 0;
    layerManager.signal_layersChanged().connect([&]() { ++layersChangedFireCount; });
    layerManager.signal_layerVisibilityChanged().connect([&]() { ++layerVisibilityChangedFireCount; });
    layerManager.signal_layerHierarchyChanged().connect([&]() { ++layerHierarchyChangedFireCount; });

    layerManager.reset();

    EXPECT_EQ(layerManager.getLayerID(_("Default")), 0) << "We should have a default layer after reset";
    EXPECT_EQ(layerManager.getLayerName(0), _("Default")) << "We should have a default layer after reset";

    EXPECT_EQ(layerManager.getLayerID("TestLayer"), -1) << "The test layer should be gone now";
    EXPECT_EQ(layerManager.getLayerID("TestLayer2"), -1) << "The test layer should be gone now";

    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "Default layer should be active";
    EXPECT_EQ(layerManager.layerIsVisible(0), true) << "Default layer should be visible";

    layerManager.foreachLayer([&](int layerId, const std::string& name)
    {
        EXPECT_EQ(layerManager.getParentLayer(layerId), -1) << "Parent relationship should have been reset";
    });

    EXPECT_EQ(layersChangedFireCount, 1) << "Layers changed signal should have been fired";
    EXPECT_EQ(layerVisibilityChangedFireCount, 1) << "Layer visibility changed signal should have been fired";
    EXPECT_EQ(layerHierarchyChangedFireCount, 1) << "Layer hierarchy changed signal should have been fired";
}

TEST_F(LayerTest, RenameLayer)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    const std::string testLayerName = "TestLayer";
    const std::string newName = "RenamedLayer";
    auto layerId = layerManager.createLayer(testLayerName);

    EXPECT_TRUE(layerManager.layerExists(layerId)) << "The test layer should have been created";
    EXPECT_EQ(layerManager.getLayerID(testLayerName), layerId) << "getLayerId reports a different ID";

    std::size_t layersChangedFireCount = 0;
    layerManager.signal_layersChanged().connect([&]()
    {
        ++layersChangedFireCount;
    });

    EXPECT_FALSE(layerManager.renameLayer(layerId, _("Default"))) << "Default name is not valid";
    EXPECT_EQ(layersChangedFireCount, 0) << "Layers changed signal should not have been fired";
    EXPECT_EQ(layerManager.getLayerID(testLayerName), layerId) << "The old name should still be valid";

    EXPECT_FALSE(layerManager.renameLayer(layerId, "")) << "Empty name is not valid";
    EXPECT_EQ(layersChangedFireCount, 0) << "Layers changed signal should not have been fired";
    EXPECT_EQ(layerManager.getLayerID(testLayerName), layerId) << "The old name should still be valid";

    // Rename the layer
    layerManager.renameLayer(layerId, newName);

    EXPECT_EQ(layersChangedFireCount, 1) << "Layers changed signal should have been fired";

    EXPECT_EQ(layerManager.getLayerID(testLayerName), -1) << "The old name should no longer be valid";
    EXPECT_EQ(layerManager.getLayerID(newName), layerId) << "The new name should now be valid";
}

TEST_F(LayerTest, GetFirstVisibleLayer)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    EXPECT_EQ(layerManager.getFirstVisibleLayer(), 0) << "The first visible layer should be the default layer after startup";

    const std::string testLayerName = "TestLayer";
    auto testLayerId = layerManager.createLayer(testLayerName);

    EXPECT_EQ(layerManager.getFirstVisibleLayer(), 0) << "The first visible layer should still be the default layer";

    // Hide the default layer, the first visible one is now the test layer
    layerManager.setLayerVisibility(0, false);
    EXPECT_EQ(layerManager.getFirstVisibleLayer(), testLayerId) << "The test layer is now the first visible one";

    layerManager.setLayerVisibility(0, true);
    EXPECT_EQ(layerManager.getFirstVisibleLayer(), 0) << "The default layer is now the first visible one again";

    // Hide the default layer again, then hide the second layer too
    layerManager.setLayerVisibility(0, false);
    EXPECT_EQ(layerManager.getFirstVisibleLayer(), testLayerId) << "The test layer is now the first visible one";
    layerManager.setLayerVisibility(testLayerId, false);

    EXPECT_EQ(getAllLayerIds(), std::vector({ 0, testLayerId })) << "No other layers should be present";
    EXPECT_EQ(layerManager.getFirstVisibleLayer(), 0) << "Even though nothing is visible, the default layer should have been returned";
}

TEST_F(LayerTest, GetActiveLayer)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "The active layer should be the default layer after startup";

    const std::string testLayerName = "TestLayer";
    auto testLayerId = layerManager.createLayer(testLayerName);

    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "The active layer should still be the default layer";

    // Hide the default layer, the active one is now the test layer
    layerManager.setLayerVisibility(0, false);
    EXPECT_EQ(layerManager.getActiveLayer(), testLayerId) << "The test layer is now active";

    layerManager.setLayerVisibility(0, true);
    EXPECT_EQ(layerManager.getActiveLayer(), testLayerId) << "The test layer remains active";

    // Hide all layers
    layerManager.setLayerVisibility(0, false);
    layerManager.setLayerVisibility(testLayerId, false);

    EXPECT_EQ(getAllLayerIds(), std::vector({ 0, testLayerId })) << "No other layers should be present";
    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "Even though nothing is visible, the default layer should be active";
}

TEST_F(LayerTest, SetActiveLayer)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "The active layer should be the default layer after startup";

    const std::string testLayerName = "TestLayer";
    auto testLayerId = layerManager.createLayer(testLayerName);

    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "The active layer should still be the default layer";

    layerManager.setActiveLayer(testLayerId);
    EXPECT_EQ(layerManager.getActiveLayer(), testLayerId) << "The test layer should now be active";

    layerManager.setActiveLayer(0);
    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "The default layer should now be active";

    // Hide all layers
    layerManager.setLayerVisibility(0, false);
    layerManager.setLayerVisibility(testLayerId, false);

    EXPECT_EQ(getAllLayerIds(), std::vector({ 0, testLayerId })) << "No other layers should be present";

    // It's still possible to set a hidden layer to active
    layerManager.setActiveLayer(0);
    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "The default layer should now be active";

    layerManager.setActiveLayer(testLayerId);
    EXPECT_EQ(layerManager.getActiveLayer(), testLayerId) << "The test layer should now be active";

    // Show the test layer, this doesn't change anything
    layerManager.setLayerVisibility(testLayerId, true);
    EXPECT_EQ(layerManager.getActiveLayer(), testLayerId) << "The test layer should now be active";

    // Showing the default layer shouldn't change the active layer
    layerManager.setLayerVisibility(0, true);
    EXPECT_EQ(layerManager.getActiveLayer(), testLayerId) << "The test layer should now be active";

    // Deleting the test layer should make the default one active
    layerManager.deleteLayer(testLayerName);
    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "The default layer should now be active";
}

TEST_F(LayerTest, LayerIsVisible)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    EXPECT_TRUE(layerManager.layerIsVisible(0)) << "The default layer should be visible";

    // It's possible to hide the default layer even if no other layer is there
    layerManager.setLayerVisibility(0, false);
    EXPECT_FALSE(layerManager.layerIsVisible(0)) << "The default layer should not be visible";

    const std::string testLayerName = "TestLayer";
    auto testLayerId = layerManager.createLayer(testLayerName);

    EXPECT_TRUE(layerManager.layerIsVisible(testLayerId)) << "The new layer should be visible after creation";
    EXPECT_FALSE(layerManager.layerIsVisible(0)) << "The default layer should not be visible";
    
    layerManager.setLayerVisibility(testLayerId, false);
    EXPECT_FALSE(layerManager.layerIsVisible(testLayerId)) << "The test layer should now be hidden";
    EXPECT_FALSE(layerManager.layerIsVisible(0)) << "The default layer should not be visible";

    // A non-existent Id should report as hidden
    EXPECT_FALSE(layerManager.layerIsVisible(-1)) << "The layer ID should report as hidden";
    EXPECT_FALSE(layerManager.layerIsVisible(testLayerId + 20)) << "A non-existent ID should report as hidden";
}

TEST_F(LayerTest, VisibilityChangedSignal)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    EXPECT_TRUE(layerManager.layerIsVisible(0)) << "The default layer should be visible";

    std::size_t layerVisibilityChangedFireCount = 0;
    std::size_t layersChangedFireCount = 0;
    layerManager.signal_layersChanged().connect([&]() { ++layersChangedFireCount; });
    layerManager.signal_layerVisibilityChanged().connect([&]() { ++layerVisibilityChangedFireCount; });

    // Trigger a visibility change
    layerManager.setLayerVisibility(0, false);

    EXPECT_EQ(layerVisibilityChangedFireCount, 1) << "Visibility changed signal should have been fired once";
    EXPECT_EQ(layersChangedFireCount, 0) << "Layers changed signal should NOT have been fired at all";

    // Hiding it again, doesn't trigger any signals
    layerVisibilityChangedFireCount = 0;
    layersChangedFireCount = 0;

    layerManager.setLayerVisibility(0, false);

    EXPECT_EQ(layerVisibilityChangedFireCount, 0) << "Visibility changed signal should not have been fired";
    EXPECT_EQ(layersChangedFireCount, 0) << "Layers changed signal should NOT have been fired at all";
}

TEST_F(LayerTest, SetLayerVisibilityAffectsActiveLayer)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto testLayerId = layerManager.createLayer("TestLayer");

    // Test how the visibility is affecting the active layer
    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "The active layer should still be the default layer";

    // Hide the default layer
    layerManager.setLayerVisibility(0, false);
    layerManager.setLayerVisibility(testLayerId, false);

    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "The test layer should now be active";

    // All layers are hidden. Showing one of them will reinstate this layer as active
    layerManager.setLayerVisibility(testLayerId, true);
    EXPECT_EQ(layerManager.getActiveLayer(), testLayerId) << "The test layer should now be active";
}

TEST_F(LayerTest, SetLayerVisibilityAffectsNode)
{
    loadMap("general_purpose.mapx");

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    // This brush is a member of layer 1
    auto brush = algorithm::findFirstBrushWithMaterial(GlobalMapModule().findOrInsertWorldspawn(), "textures/numbers/4");

    EXPECT_TRUE(brush->visible()) << "Brush should be visible";

    auto layers = brush->getLayers();
    EXPECT_NE(layers.find(1), layers.end()) << "Brush should be a member of layer 1";

    layerManager.setLayerVisibility(1, false);
    EXPECT_FALSE(brush->visible()) << "Brush should be hidden now";

    layerManager.setLayerVisibility(1, true);
    EXPECT_TRUE(brush->visible()) << "Brush should be visible again";
}

TEST_F(LayerTest, SetLayerParent)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto parentLayerId = layerManager.createLayer("ParentLayer");
    auto childLayerId = layerManager.createLayer("ChildLayer");

    EXPECT_EQ(layerManager.getParentLayer(0), -1) << "Default layer cannot have a parent";
    EXPECT_EQ(layerManager.getParentLayer(parentLayerId), -1) << "ParentLayer doesn't have a parent yet";
    EXPECT_EQ(layerManager.getParentLayer(childLayerId), -1) << "ChildLayer doesn't have a parent yet";

    // It's not valid to assign a parent to the default layer, unless it's -1
    EXPECT_NO_THROW(layerManager.setParentLayer(0, -1));
    EXPECT_THROW(layerManager.setParentLayer(0, parentLayerId), std::invalid_argument);
    EXPECT_THROW(layerManager.setParentLayer(0, 3333), std::invalid_argument);

    // It's not allowed to assign a layer as its own parent
    EXPECT_THROW(layerManager.setParentLayer(parentLayerId, parentLayerId), std::invalid_argument);

    // Assigning an invalid parent to a valid child layer throws
    EXPECT_THROW(layerManager.setParentLayer(childLayerId, 2222), std::invalid_argument);
    // Assigning a valid parent to an invalid child layer throws
    EXPECT_THROW(layerManager.setParentLayer(2222, parentLayerId), std::invalid_argument);

    // This is a valid operation
    EXPECT_NO_THROW(layerManager.setParentLayer(childLayerId, parentLayerId));
    EXPECT_EQ(layerManager.getParentLayer(childLayerId), parentLayerId) << "Parent Id has not been assigned";
    // It's ok to do it again
    EXPECT_NO_THROW(layerManager.setParentLayer(childLayerId, parentLayerId));

    // Remove the parent again
    EXPECT_NO_THROW(layerManager.setParentLayer(childLayerId, -1));
    EXPECT_EQ(layerManager.getParentLayer(childLayerId), -1) << "Parent Id has not been removed";
    // It's ok to do it again
    EXPECT_NO_THROW(layerManager.setParentLayer(childLayerId, -1));
    EXPECT_EQ(layerManager.getParentLayer(childLayerId), -1) << "Parent Id has not been removed";

    // Assign a parentLayer, then reassign it to a different parent
    EXPECT_NO_THROW(layerManager.setParentLayer(childLayerId, parentLayerId));
    EXPECT_NO_THROW(layerManager.setParentLayer(childLayerId, 0));
    EXPECT_EQ(layerManager.getParentLayer(childLayerId), 0) << "Parent Id has not been reassigned";
}

TEST_F(LayerTest, HierarchyChangedSignal)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    auto testLayerId = layerManager.createLayer("TestLayer");
    auto testLayer2Id = layerManager.createLayer("TestLayer2");

    std::size_t layerVisibilityChangedFireCount = 0;
    std::size_t layersChangedFireCount = 0;
    std::size_t layerHierarchyChangedFireCount = 0;
    layerManager.signal_layersChanged().connect([&]() { ++layersChangedFireCount; });
    layerManager.signal_layerVisibilityChanged().connect([&]() { ++layerVisibilityChangedFireCount; });
    layerManager.signal_layerHierarchyChanged().connect([&]() { ++layerHierarchyChangedFireCount; });

    // Trigger a hierarchy change
    layerManager.setParentLayer(testLayer2Id, testLayerId);

    EXPECT_EQ(layerHierarchyChangedFireCount, 1) << "Hierarchy changed signal should have been fired once";
    EXPECT_EQ(layerVisibilityChangedFireCount, 0) << "Visibility changed signal should not have been fired";
    EXPECT_EQ(layersChangedFireCount, 0) << "Layers changed signal should NOT have been fired at all";

    // Hiding it again, doesn't trigger any signals
    layerHierarchyChangedFireCount = 0;
    layerVisibilityChangedFireCount = 0;
    layersChangedFireCount = 0;

    layerManager.setParentLayer(testLayer2Id, testLayerId);

    EXPECT_EQ(layerHierarchyChangedFireCount, 0) << "Hierarchy changed signal should not have been fired";
    EXPECT_EQ(layerVisibilityChangedFireCount, 0) << "Visibility changed signal should not have been fired";
    EXPECT_EQ(layersChangedFireCount, 0) << "Layers changed signal should NOT have been fired at all";

    layerManager.setParentLayer(testLayer2Id, 0);
    layerManager.setParentLayer(testLayer2Id, -1);

    EXPECT_EQ(layerHierarchyChangedFireCount, 2) << "Hierarchy changed signal should have been fired twice";
    EXPECT_EQ(layerVisibilityChangedFireCount, 0) << "Visibility changed signal should not have been fired";
    EXPECT_EQ(layersChangedFireCount, 0) << "Layers changed signal should NOT have been fired at all";
}

void runLayerHierarchyPersistenceTest(const std::string& mapFilePath)
{
    GlobalMapModule().findOrInsertWorldspawn(); // to not save an empty map

    TemporaryFile tempSavedFile(mapFilePath);

    auto* layerManager = &GlobalMapModule().getRoot()->getLayerManager();
    auto parentLayerId = layerManager->createLayer("ParentLayer");
    auto childLayerId = layerManager->createLayer("ChildLayer");
    auto childLayer2Id = layerManager->createLayer("ChildLayer2");
    auto childLayer3Id = layerManager->createLayer("ChildLayer3");

    // Default > Parent > Child 
    layerManager->setParentLayer(childLayerId, parentLayerId);
    layerManager->setParentLayer(parentLayerId, 0);

    // Default > Child2
    layerManager->setParentLayer(childLayer2Id, 0);

    // Child3 remains a top-level layer

    EXPECT_FALSE(os::fileOrDirExists(mapFilePath));
    GlobalCommandSystem().executeCommand("SaveMapCopyAs", mapFilePath);

    // Clear the current map, discarding the changes
    FileSaveConfirmationHelper helper(radiant::FileSaveConfirmation::Action::DiscardChanges);
    GlobalCommandSystem().executeCommand("NewMap");

    // The layers are gone now
    EXPECT_EQ(getAllLayerIds(), std::vector{ 0 }) << "All layers should have been cleared, except for the default";

    // Then load the map from that temporary path
    EXPECT_TRUE(os::fileOrDirExists(mapFilePath));
    GlobalCommandSystem().executeCommand("OpenMap", mapFilePath);

    auto unsortedLayerIds = getAllLayerIds();
    auto layerIds = std::set(unsortedLayerIds.begin(), unsortedLayerIds.end());
    EXPECT_EQ(layerIds.count(0), 1) << "Default layer ID not present";
    EXPECT_EQ(layerIds.count(parentLayerId), 1) << "Parent Layer ID not present";
    EXPECT_EQ(layerIds.count(childLayerId), 1) << "Child Layer ID not present";
    EXPECT_EQ(layerIds.count(childLayer2Id), 1) << "Child Layer 2 ID not present";
    EXPECT_EQ(layerIds.count(childLayer3Id), 1) << "Child Layer 3 ID not present";

    // We changed maps, so acquire a new layer manager reference
    layerManager = &GlobalMapModule().getRoot()->getLayerManager();
    EXPECT_EQ(layerManager->getParentLayer(childLayerId), parentLayerId) << "Hierarchy has not been restored";
    EXPECT_EQ(layerManager->getParentLayer(parentLayerId), 0) << "Hierarchy has not been restored";
    EXPECT_EQ(layerManager->getParentLayer(childLayer2Id), 0) << "Hierarchy has not been restored";
    EXPECT_EQ(layerManager->getParentLayer(childLayer3Id), -1) << "Hierarchy has not been restored";
}

TEST_F(LayerTest, LayerHierarchyIsPersistedToMapx)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "layer_hierarchy_test.mapx";

    runLayerHierarchyPersistenceTest(tempPath.string());
}

TEST_F(LayerTest, LayerHierarchyIsPersistedToMap)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "layer_hierarchy_test.map";
    // Remove the .darkradiant file after this test
    TemporaryFile tempDarkRadiantFile(os::replaceExtension(tempPath.string(), "darkradiant"));

    runLayerHierarchyPersistenceTest(tempPath.string());
}

TEST_F(LayerTest, LayerHierarchyIsRestoredFromMapx)
{
    // The .mapx file contains a hierarchy that should be restored
    // without running into crashes (layer 3 is a child of layer 9)
    auto mapFilePath = _context.getTestProjectPath() + "maps/layer_hierarchy_restore.mapx";

    GlobalCommandSystem().executeCommand("OpenMap", mapFilePath);

    auto unsortedLayerIds = getAllLayerIds();
    auto layerIds = std::set(unsortedLayerIds.begin(), unsortedLayerIds.end());

    for (int i = 0; i <= 9; ++i)
    {
        EXPECT_EQ(layerIds.count(0), 1) << "Layer with ID " << i << " not present";
    }

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    EXPECT_EQ(layerManager.getParentLayer(0), -1) << "Hierarchy has not been restored";
    EXPECT_EQ(layerManager.getParentLayer(3), 9) << "Hierarchy has not been restored";
    EXPECT_EQ(layerManager.getParentLayer(4), 2) << "Hierarchy has not been restored";
    EXPECT_EQ(layerManager.getParentLayer(5), 3) << "Hierarchy has not been restored";
    EXPECT_EQ(layerManager.getParentLayer(9), 2) << "Hierarchy has not been restored";
}

TEST_F(LayerTest, LayerHierarchyIsRestoredFromMap)
{
    // The .darkradiant file contains a hierarchy that should be restored
    // without running into crashes (layer 3 is a child of layer 9)
    auto mapFilePath = _context.getTestProjectPath() + "maps/layer_hierarchy_restore.map";

    GlobalCommandSystem().executeCommand("OpenMap", mapFilePath);

    auto unsortedLayerIds = getAllLayerIds();
    auto layerIds = std::set(unsortedLayerIds.begin(), unsortedLayerIds.end());

    for (int i = 0; i <= 9; ++i)
    {
        EXPECT_EQ(layerIds.count(0), 1) << "Layer with ID " << i << " not present";
    }

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    EXPECT_EQ(layerManager.getParentLayer(0), -1) << "Hierarchy has not been restored";
    EXPECT_EQ(layerManager.getParentLayer(3), 9) << "Hierarchy has not been restored";
    EXPECT_EQ(layerManager.getParentLayer(4), 2) << "Hierarchy has not been restored";
    EXPECT_EQ(layerManager.getParentLayer(5), 3) << "Hierarchy has not been restored";
    EXPECT_EQ(layerManager.getParentLayer(9), 2) << "Hierarchy has not been restored";
}

TEST_F(LayerTest, CreateLayerMarksMapAsModified)
{
    loadMap("general_purpose.mapx");

    EXPECT_FALSE(GlobalMapModule().isModified());

    GlobalCommandSystem().executeCommand("CreateLayer", cmd::Argument("TestLayer"));

    EXPECT_TRUE(GlobalMapModule().isModified());
}

TEST_F(LayerTest, RenameLayerMarksMapAsModified)
{
    loadMap("general_purpose.mapx");

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    EXPECT_FALSE(GlobalMapModule().isModified());
    auto layerId = layerManager.getLayerID("Second Layer");
    EXPECT_NE(layerId, -1);

    GlobalCommandSystem().executeCommand("RenameLayer", cmd::Argument(layerId), cmd::Argument("Renamed Layer"));

    EXPECT_TRUE(GlobalMapModule().isModified());
}

TEST_F(LayerTest, DeleteLayerMarksMapAsModified)
{
    loadMap("general_purpose.mapx");

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    EXPECT_FALSE(GlobalMapModule().isModified());
    auto layerId = layerManager.getLayerID("Second Layer");
    EXPECT_NE(layerId, -1);

    GlobalCommandSystem().executeCommand("DeleteLayer", cmd::Argument(layerId));

    EXPECT_TRUE(GlobalMapModule().isModified());
}

enum class LayerAction
{
    AddToLayer,
    RemoveFromLayer,
    MoveToLayer,
};

void performMoveOrAddToLayerTest(LayerAction action)
{
    auto brush = algorithm::findFirstBrushWithMaterial(
        GlobalMapModule().findOrInsertWorldspawn(), "textures/numbers/1");
    EXPECT_TRUE(brush);

    Node_setSelected(brush, true);

    EXPECT_FALSE(GlobalMapModule().isModified());

    auto layerId = GlobalMapModule().getRoot()->getLayerManager().getLayerID("Second Layer");
    EXPECT_NE(layerId, -1);

    switch (action)
    {
    case LayerAction::AddToLayer:
        GlobalCommandSystem().executeCommand("AddSelectionToLayer", cmd::Argument(layerId));
        break;
    case LayerAction::MoveToLayer:
        GlobalCommandSystem().executeCommand("MoveSelectionToLayer", cmd::Argument(layerId));
        break;
    case LayerAction::RemoveFromLayer:
        GlobalCommandSystem().executeCommand("RemoveSelectionFromLayer", cmd::Argument(layerId));
        break;
    }

    EXPECT_TRUE(GlobalMapModule().isModified());
}

TEST_F(LayerTest, AddingToLayerMarksMapAsModified)
{
    loadMap("general_purpose.mapx");

    performMoveOrAddToLayerTest(LayerAction::AddToLayer);
}

TEST_F(LayerTest, MovingToLayerMarksMapAsModified)
{
    loadMap("general_purpose.mapx");

    performMoveOrAddToLayerTest(LayerAction::MoveToLayer);
}

TEST_F(LayerTest, RemovingFromLayerMarksMapAsModified)
{
    loadMap("general_purpose.mapx");

    performMoveOrAddToLayerTest(LayerAction::RemoveFromLayer);
}

}
