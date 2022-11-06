#include "i18n.h"
#include "RadiantTest.h"

#include "imap.h"
#include "ilayer.h"
#include "ifilter.h"
#include "scenelib.h"
#include "algorithm/Primitives.h"
#include "os/file.h"

#include "algorithm/Scene.h"
#include "string/split.h"
#include "string/trim.h"
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

TEST_F(LayerTest, SetLayerVisibilityUsingInvalidId)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    // Hide non-existent layers
    EXPECT_NO_THROW(layerManager.setLayerVisibility(333, false));
    EXPECT_NO_THROW(layerManager.setLayerVisibility(-2, false));
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

TEST_F(LayerTest, SetLayerVisibilityWorksRecursively)
{
    loadMap("layer_hierarchy_restore.mapx");

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto testLayer2Id = layerManager.getLayerID("Test2");
    auto testLayer7Id = layerManager.getLayerID("Test7");
    auto boardsLayerId = layerManager.getLayerID("BoardsandStuff");
    EXPECT_EQ(layerManager.getParentLayer(testLayer2Id), boardsLayerId) << "Test setup is wrong";
    EXPECT_EQ(layerManager.getParentLayer(testLayer7Id), testLayer2Id) << "Test setup is wrong";

    // Move the worldspawn to the grand-child layer
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    Node_setSelected(worldspawn, true);
    layerManager.moveSelectionToLayer(testLayer7Id);
    Node_setSelected(worldspawn, false);

    EXPECT_TRUE(worldspawn->visible());

    // Hide the parent layer, this should affect the child layer
    layerManager.setLayerVisibility(boardsLayerId, false);
    EXPECT_FALSE(worldspawn->visible()) << "Worldspawn should be hidden now";

    EXPECT_FALSE(layerManager.layerIsVisible(testLayer7Id)) <<
        "The parent layer visibility should have propagated down to the boards layer";
    EXPECT_FALSE(layerManager.layerIsVisible(testLayer2Id)) <<
        "The parent layer visibility should have propagated down to the boards layer";

    layerManager.setLayerVisibility(boardsLayerId, true);
    EXPECT_TRUE(worldspawn->visible()) << "Worldspawn should be visible again";

    EXPECT_TRUE(layerManager.layerIsVisible(testLayer7Id)) <<
        "The parent layer visibility should have propagated down to the boards layer";
    EXPECT_TRUE(layerManager.layerIsVisible(testLayer2Id)) <<
        "The parent layer visibility should have propagated down to the boards layer";
}

TEST_F(LayerTest, GetParentLayer)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    EXPECT_EQ(layerManager.getParentLayer(0), -1) << "Default layer doesn't have a parent";

    // Set a parent, query again
    auto someLayerId = layerManager.createLayer("SomeLayer");
    layerManager.setParentLayer(someLayerId, 0);
    EXPECT_EQ(layerManager.getParentLayer(someLayerId), 0);

    // Layer ID -1 is not throwing
    EXPECT_NO_THROW(layerManager.getParentLayer(-1));

    // Any other invalid ID should throw
    EXPECT_THROW(layerManager.getParentLayer(3434), std::out_of_range);
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

TEST_F(LayerTest, SetLayerParentRecursionDetection)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto grandParentLayerId = layerManager.createLayer("GrandParentLayer");
    auto parentLayerId = layerManager.createLayer("ParentLayer");
    auto childLayerId = layerManager.createLayer("ChildLayer");

    EXPECT_EQ(layerManager.getParentLayer(0), -1) << "Default layer cannot have a parent";
    EXPECT_EQ(layerManager.getParentLayer(grandParentLayerId), -1) << "GrandParentLayer doesn't have a parent yet";
    EXPECT_EQ(layerManager.getParentLayer(parentLayerId), -1) << "ParentLayer doesn't have a parent yet";
    EXPECT_EQ(layerManager.getParentLayer(childLayerId), -1) << "ChildLayer doesn't have a parent yet";

    // Form a straight line GrandParent > Parent > Child
    layerManager.setParentLayer(childLayerId, parentLayerId);
    layerManager.setParentLayer(parentLayerId, grandParentLayerId);

    // Then try to assign Child as a parent to its own GrandParent => recursion detection should throw
    EXPECT_THROW(layerManager.setParentLayer(grandParentLayerId, childLayerId), std::invalid_argument);
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

void loadAndExpectLayersToBeRestored(const std::string& mapFilePath, const std::vector<int>& expectedLayerIds)
{
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

    EXPECT_EQ(layerIds.size(), expectedLayerIds.size()) << "Expected layer count not matching";

    for (auto expectedId : expectedLayerIds)
    {
        EXPECT_EQ(layerIds.count(expectedId), 1) << "Layer ID " << expectedId << " not present";
    }
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

    loadAndExpectLayersToBeRestored(mapFilePath, { 0, parentLayerId, childLayerId, childLayer2Id, childLayer3Id });

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

TEST_F(LayerTest, LayerIsChildOf)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto grandParentLayerId = layerManager.createLayer("GrandParentLayer");
    auto parentLayerId = layerManager.createLayer("ParentLayer");
    auto childLayerId = layerManager.createLayer("ChildLayer");
    auto otherLayerId = layerManager.createLayer("OtherLayer");

    // Form a straight line GrandParent > Parent > Child
    layerManager.setParentLayer(childLayerId, parentLayerId);
    layerManager.setParentLayer(parentLayerId, grandParentLayerId);

    EXPECT_TRUE(layerManager.layerIsChildOf(childLayerId, parentLayerId)) << "Child should be a descendent of parent";
    EXPECT_TRUE(layerManager.layerIsChildOf(childLayerId, grandParentLayerId)) << "Child should be a descendent of grand parent";

    EXPECT_FALSE(layerManager.layerIsChildOf(-1, parentLayerId)) << "One ID is -1, should return false";
    EXPECT_FALSE(layerManager.layerIsChildOf(childLayerId, -1)) << "One ID is -1, should return false";
    EXPECT_FALSE(layerManager.layerIsChildOf(-1, -1)) << "Both ids are -1, should return false";

    EXPECT_FALSE(layerManager.layerIsChildOf(childLayerId, childLayerId)) << "Both ids are the same, should return false";

    EXPECT_FALSE(layerManager.layerIsChildOf(otherLayerId, parentLayerId)) << "OtherLayer is not a descendent of parent";
    EXPECT_FALSE(layerManager.layerIsChildOf(otherLayerId, grandParentLayerId)) << "OtherLayer is not a descendent of parent";
    EXPECT_FALSE(layerManager.layerIsChildOf(otherLayerId, 0)) << "OtherLayer is not a descendent of parent";

    EXPECT_FALSE(layerManager.layerIsChildOf(0, -1)) << "Even though Default has no parent, this should return -1";
    EXPECT_FALSE(layerManager.layerIsChildOf(0, grandParentLayerId)) << "Default is not a descendant of grand parent";
}

TEST_F(LayerTest, SelectLayer)
{
    loadMap("layer_hierarchy_restore.mapx");

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto testLayer2Id = layerManager.getLayerID("Test2");
    
    // Create brushes and move them into layers
    auto brush1 = algorithm::createCubicBrush(GlobalMapModule().findOrInsertWorldspawn(), { 200, 300, 100 }, "textures/numbers/1");
    auto brush2 = algorithm::createCubicBrush(GlobalMapModule().findOrInsertWorldspawn(), { 200, 300, 100 }, "textures/numbers/2");
    auto brush3 = algorithm::createCubicBrush(GlobalMapModule().findOrInsertWorldspawn(), { 200, 300, 100 }, "textures/numbers/3");
    auto brush4 = algorithm::createCubicBrush(GlobalMapModule().findOrInsertWorldspawn(), { 200, 300, 100 }, "textures/numbers/4");

    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    layerManager.moveSelectionToLayer(testLayer2Id);
    Node_setSelected(brush1, false);
    Node_setSelected(brush2, false);

    EXPECT_FALSE(Node_isSelected(brush1));
    EXPECT_FALSE(Node_isSelected(brush2));
    EXPECT_FALSE(Node_isSelected(brush3));
    Node_setSelected(brush4, true); // this selection status should survive the process

    // Use the layer interface to select test layer 2
    layerManager.setSelected(testLayer2Id, true);

    EXPECT_TRUE(Node_isSelected(brush1)) << "Brush 1 should be selected";
    EXPECT_TRUE(Node_isSelected(brush2)) << "Brush 2 should be selected";
    EXPECT_FALSE(Node_isSelected(brush3)) << "Brush 3 should still be unselected";
    EXPECT_TRUE(Node_isSelected(brush4)) << "Brush 4 should remain selected";

    // De-select the layer again
    layerManager.setSelected(testLayer2Id, false);

    EXPECT_FALSE(Node_isSelected(brush1)) << "Brush 1 should not be selected anymore";
    EXPECT_FALSE(Node_isSelected(brush2)) << "Brush 2 should not be selected anymore";
    EXPECT_FALSE(Node_isSelected(brush3)) << "Brush 3 should still be unselected";
    EXPECT_TRUE(Node_isSelected(brush4)) << "Brush 4 should remain selected";
}

TEST_F(LayerTest, SelectLayerWorksRecursively)
{
    loadMap("layer_hierarchy_restore.mapx");

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto testLayer2Id = layerManager.getLayerID("Test2");
    auto testLayer7Id = layerManager.getLayerID("Test7");
    auto boardsLayerId = layerManager.getLayerID("BoardsandStuff");
    EXPECT_EQ(layerManager.getParentLayer(testLayer2Id), boardsLayerId) << "Test setup is wrong";
    EXPECT_EQ(layerManager.getParentLayer(testLayer7Id), testLayer2Id) << "Test setup is wrong";

    // Create brushes and move them into the layer hierarchy
    auto brush1 = algorithm::createCubicBrush(GlobalMapModule().findOrInsertWorldspawn(), { 200, 300, 100 }, "textures/numbers/1");
    auto brush2 = algorithm::createCubicBrush(GlobalMapModule().findOrInsertWorldspawn(), { 200, 300, 100 }, "textures/numbers/2");
    auto brush3 = algorithm::createCubicBrush(GlobalMapModule().findOrInsertWorldspawn(), { 200, 300, 100 }, "textures/numbers/3");
    auto brush4 = algorithm::createCubicBrush(GlobalMapModule().findOrInsertWorldspawn(), { 200, 300, 100 }, "textures/numbers/4");

    // Brushes 1 and 2 go to test layer 7
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    layerManager.moveSelectionToLayer(testLayer7Id);
    Node_setSelected(brush1, false);
    Node_setSelected(brush2, false);

    // Brush 3 goes to the parent layer Test2
    Node_setSelected(brush3, true);
    layerManager.moveSelectionToLayer(testLayer2Id);
    Node_setSelected(brush3, false);

    // Brush 4 remains in the grand parent layer
    Node_setSelected(brush4, true);
    layerManager.moveSelectionToLayer(boardsLayerId);
    Node_setSelected(brush4, false);

    EXPECT_FALSE(Node_isSelected(brush1));
    EXPECT_FALSE(Node_isSelected(brush2));
    EXPECT_FALSE(Node_isSelected(brush3));
    EXPECT_FALSE(Node_isSelected(brush4));

    // Use the layer interface to select the grand parent, this should affect all child layers
    layerManager.setSelected(boardsLayerId, true);

    EXPECT_TRUE(Node_isSelected(brush1)) << "Brush 1 should be selected";
    EXPECT_TRUE(Node_isSelected(brush2)) << "Brush 2 should be selected";
    EXPECT_TRUE(Node_isSelected(brush3)) << "Brush 3 should be unselected";
    EXPECT_TRUE(Node_isSelected(brush4)) << "Brush 4 should be selected";

    // De-select the layer again
    layerManager.setSelected(boardsLayerId, false);

    EXPECT_FALSE(Node_isSelected(brush1)) << "Brush 1 should not be selected anymore";
    EXPECT_FALSE(Node_isSelected(brush2)) << "Brush 2 should not be selected anymore";
    EXPECT_FALSE(Node_isSelected(brush3)) << "Brush 3 should not be selected anymore";
    EXPECT_FALSE(Node_isSelected(brush4)) << "Brush 4 should not be selected anymore";
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

TEST_F(LayerTest, SettingParentLayerMarksMapAsModified)
{
    loadMap("general_purpose.mapx");

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto secondLayerId = layerManager.getLayerID("Second Layer");
    auto thirdLayerId = layerManager.getLayerID("Third Layer");

    EXPECT_FALSE(GlobalMapModule().isModified());

    layerManager.setParentLayer(thirdLayerId, secondLayerId);

    EXPECT_TRUE(GlobalMapModule().isModified());
}

// #6115: Selecting and deselecting a filtered child brush through layers leaves the brush selected
TEST_F(LayerTest, SelectFilteredChildPrimitive)
{
    loadMap("selecting_filtered_items_with_layers.mapx");

    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();
    auto caulkLayerId = layerManager.getLayerID("Caulk");

    auto func_static_1 = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");
    auto childPrimitive = algorithm::findFirstBrushWithMaterial(func_static_1, "textures/common/caulk");

    EXPECT_EQ(childPrimitive->getLayers(), scene::LayerList{ caulkLayerId }) << "Unexpected layer setup";
    EXPECT_TRUE(childPrimitive->visible()) << "The brush should be visible after map load";

    // Activate the filter hiding the caulk brush
    GlobalFilterSystem().setFilterState("Caulk", true);

    EXPECT_FALSE(childPrimitive->visible()) << "The filter should have hidden the brush";

    // Now select the layer, this should set the brush to visible
    // because the parent entity selection implies a forced-visible state on the brush
    layerManager.setSelected(caulkLayerId, true);

    EXPECT_TRUE(childPrimitive->visible()) << "Selecting the layer should set the brush to visible";

    EXPECT_TRUE(Node_isSelected(func_static_1)) << "Selecting the layer should have selected func_static_1";
    EXPECT_TRUE(Node_isSelected(childPrimitive)) << "Selecting the layer selected the brush since it was forced visible";

    layerManager.setSelected(caulkLayerId, false);

    EXPECT_FALSE(Node_isSelected(func_static_1)) << "De-selecting the layer should have de-selected func_static_1";
    EXPECT_FALSE(Node_isSelected(childPrimitive)) << "De-selecting the layer shouldn't leave the brush selected";
}

void runLayerVisibilityPersistenceTest(const std::string& mapFilePath)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn(); // to not save an empty map

    // Create two brushes
    auto brush1 = algorithm::createCubicBrush(worldspawn);
    Node_getIBrush(brush1)->setShader("textures/numbers/1");
    auto brush2 = algorithm::createCubicBrush(worldspawn);
    Node_getIBrush(brush2)->setShader("textures/numbers/2");

    TemporaryFile tempSavedFile(mapFilePath);

    auto* layerManager = &GlobalMapModule().getRoot()->getLayerManager();
    auto layer1Id = layerManager->createLayer("Layer1");
    auto layer2Id = layerManager->createLayer("Layer2");
    auto layer3Id = layerManager->createLayer("Layer3");
    auto layer4Id = layerManager->createLayer("Layer4");

    // Brush 1 goes into Layer 1
    Node_setSelected(brush1, true);
    layerManager->moveSelectionToLayer(layer1Id);
    Node_setSelected(brush1, false);

    // Brush 2 goes into Layer 2
    Node_setSelected(brush2, true);
    layerManager->moveSelectionToLayer(layer2Id);
    Node_setSelected(brush2, false);

    // Set layers 1 and 3 to hidden
    layerManager->setLayerVisibility(layer1Id, false);
    layerManager->setLayerVisibility(layer3Id, false);

    EXPECT_FALSE(os::fileOrDirExists(mapFilePath));
    GlobalCommandSystem().executeCommand("SaveMapCopyAs", mapFilePath);

    // Prevent crashes when switching maps while holding strong refs to nodes
    worldspawn.reset();
    brush1.reset();
    brush2.reset();

    loadAndExpectLayersToBeRestored(mapFilePath, { 0, layer1Id, layer2Id, layer3Id, layer4Id });

    // We changed maps, so acquire a new layer manager reference
    layerManager = &GlobalMapModule().getRoot()->getLayerManager();
    EXPECT_FALSE(layerManager->layerIsVisible(layer1Id)) << "Visibility has not been restored";
    EXPECT_TRUE(layerManager->layerIsVisible(layer2Id)) << "Visibility has not been restored";
    EXPECT_FALSE(layerManager->layerIsVisible(layer3Id)) << "Visibility has not been restored";
    EXPECT_TRUE(layerManager->layerIsVisible(layer4Id)) << "Visibility has not been restored";

    // Expect that brush 1 is invisible, brush 2 should be shown
    worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");

    EXPECT_FALSE(brush1->visible()) << "Brush 1 should be invisible since Layer 1 is hidden";
    EXPECT_TRUE(brush2->visible()) << "Brush 2 should be visible since Layer 2 is visible";

    EXPECT_TRUE(brush1->checkStateFlag(scene::Node::eLayered)) << "Brush 1 should be invisible due to layering";
    EXPECT_FALSE(brush2->checkStateFlag(scene::Node::eLayered)) << "Brush 2 should not be invisible due to layering";
}

TEST_F(LayerTest, LayerVisibilityIsPersistedToMap)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "layer_visibility_test.map";

    // Remove the .darkradiant file after this test
    TemporaryFile tempDarkRadiantFile(os::replaceExtension(tempPath.string(), "darkradiant"));

    runLayerVisibilityPersistenceTest(tempPath.string());
}

TEST_F(LayerTest, LayerVisibilityIsPersistedToMapx)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "layer_visibility_test.mapx";

    runLayerVisibilityPersistenceTest(tempPath.string());
}

void runLayerActiveStatusPersistenceTest(const std::string& mapFilePath)
{
    GlobalMapModule().findOrInsertWorldspawn(); // to not save an empty map

    TemporaryFile tempSavedFile(mapFilePath);

    auto* layerManager = &GlobalMapModule().getRoot()->getLayerManager();
    auto layer1Id = layerManager->createLayer("Layer1");
    auto layer2Id = layerManager->createLayer("Layer2");

    EXPECT_EQ(layerManager->getActiveLayer(), 0) << "Default layer should be active";

    // Set layer 2 to active
    layerManager->setActiveLayer(layer2Id);

    EXPECT_FALSE(os::fileOrDirExists(mapFilePath));
    GlobalCommandSystem().executeCommand("SaveMapCopyAs", mapFilePath);

    loadAndExpectLayersToBeRestored(mapFilePath, { 0, layer1Id, layer2Id });

    // We changed maps, so acquire a new layer manager reference
    layerManager = &GlobalMapModule().getRoot()->getLayerManager();
    EXPECT_EQ(layerManager->getActiveLayer(), layer2Id) << "Active layer has not been restored";
}

TEST_F(LayerTest, LayerActiveStatusIsPersistedToMap)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "layer_active_status_test.map";

    // Remove the .darkradiant file after this test
    TemporaryFile tempDarkRadiantFile(os::replaceExtension(tempPath.string(), "darkradiant"));

    runLayerActiveStatusPersistenceTest(tempPath.string());
}

TEST_F(LayerTest, LayerActiveStatusIsPersistedToMapx)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "layer_active_status_test.mapx";

    runLayerActiveStatusPersistenceTest(tempPath.string());
}

inline void replacePropertiesBlock(const std::string& infoFilePath, const std::string& replacement)
{
    auto contents = algorithm::loadFileToString(infoFilePath);

    std::list<std::string> lines;
    string::split(lines, contents, "\n\r");

    contents.clear();
    for (auto line = lines.begin(); line != lines.end(); ++line)
    {
        if (string::trim_copy(*line) == "LayerProperties")
        {
            ++line; // skip LayerProperties
            ++line; // skip the opening brace

            while (string::trim_copy(*line) != "}")
            {
                ++line;
            }

            // Add the replacement block here
            contents.append(replacement);
            contents.append("\n");
            continue;
        }

        contents.append(*line);
        contents.append("\n");
    }

    algorithm::replaceFileContents(infoFilePath, contents);
}

TEST_F(LayerTest, ParseEmptyLayerPropertiesBlock)
{
    fs::path mapFilePath = _context.getTestProjectPath();
    mapFilePath /= "maps/layer_hierarchy_restore.map";

    // Restore the .darkradiant file after this test
    auto infoFilePath = os::replaceExtension(mapFilePath.string(), "darkradiant");
    BackupCopy copy(infoFilePath);

    replacePropertiesBlock(infoFilePath, R"(    LayerProperties
    {
    })");

    loadAndExpectLayersToBeRestored(mapFilePath.string(), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getParentLayer(3), 9) << "Failed to restore hierarchy";
}

TEST_F(LayerTest, ParseLayerPropertiesActiveLayer)
{
    fs::path mapFilePath = _context.getTestProjectPath();
    mapFilePath /= "maps/layer_hierarchy_restore.map";

    // Restore the .darkradiant file after this test
    auto infoFilePath = os::replaceExtension(mapFilePath.string(), "darkradiant");
    BackupCopy backupCopy(infoFilePath);

    // Throw in a few potentially problematic cases.
    // The hierarchy should still restore as planned
    replacePropertiesBlock(infoFilePath, R"(LayerProperties { ActiveLayer { -2 } })");
    loadAndExpectLayersToBeRestored(mapFilePath.string(), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getParentLayer(3), 9) << "Failed to restore hierarchy";
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getActiveLayer(), 0) << "Active layer should be default";
    backupCopy.restoreNow();

    replacePropertiesBlock(infoFilePath, R"(LayerProperties { ActiveLayer { 36545634 } })");
    loadAndExpectLayersToBeRestored(mapFilePath.string(), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getParentLayer(3), 9) << "Failed to restore hierarchy";
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getActiveLayer(), 0) << "Active layer should be default";
    backupCopy.restoreNow();

    replacePropertiesBlock(infoFilePath, R"(LayerProperties { ActiveLayer { abc } })");
    loadAndExpectLayersToBeRestored(mapFilePath.string(), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getParentLayer(3), 9) << "Failed to restore hierarchy";
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getActiveLayer(), 0) << "Active layer should be default";
    backupCopy.restoreNow();

    // Now the positive test case
    replacePropertiesBlock(infoFilePath, R"(LayerProperties { ActiveLayer { 3 } })");
    loadAndExpectLayersToBeRestored(mapFilePath.string(), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getParentLayer(3), 9) << "Failed to restore hierarchy";
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getActiveLayer(), 3) << "Failed to restore active layer";
    backupCopy.restoreNow();
}

void expectLayersAreVisible(const std::vector<int>& layerIds, bool expectVisible)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    for (auto id : layerIds)
    {
        EXPECT_EQ(layerManager.layerIsVisible(id), expectVisible) << "Layer ID " << id << " didn't meet visibility expectation";
    }
}

TEST_F(LayerTest, ParseLayerPropertiesHiddenLayers)
{
    fs::path mapFilePath = _context.getTestProjectPath();
    mapFilePath /= "maps/layer_hierarchy_restore.map";

    // Restore the .darkradiant file after this test
    auto infoFilePath = os::replaceExtension(mapFilePath.string(), "darkradiant");
    BackupCopy backupCopy(infoFilePath);

    // No hidden layers
    replacePropertiesBlock(infoFilePath, R"(LayerProperties { HiddenLayers { } })");
    loadAndExpectLayersToBeRestored(mapFilePath.string(), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getParentLayer(3), 9) << "Failed to restore hierarchy";
    expectLayersAreVisible({ 0,1,2,3,4,5,6,7,8,9 }, true); // all layers should be visible
    backupCopy.restoreNow();

    // An invalid layer ID in there
    replacePropertiesBlock(infoFilePath, R"(LayerProperties { HiddenLayers { -1 3 4 } })");
    loadAndExpectLayersToBeRestored(mapFilePath.string(), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getParentLayer(3), 9) << "Failed to restore hierarchy";
    expectLayersAreVisible({ 0,1,2,5,6,7,8,9 }, true);
    expectLayersAreVisible({ 3, 4 }, false);
    backupCopy.restoreNow();

    // Duplicate layer IDs listed, plus one out of range
    replacePropertiesBlock(infoFilePath, R"(LayerProperties { HiddenLayers { 0 4 5 5 5 3 44444 } })");
    loadAndExpectLayersToBeRestored(mapFilePath.string(), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getParentLayer(3), 9) << "Failed to restore hierarchy";
    expectLayersAreVisible({ 1,2,6,7,8,9 }, true);
    expectLayersAreVisible({ 0,3,4,5 }, false);
    backupCopy.restoreNow();

    // Now the positive test case
    replacePropertiesBlock(infoFilePath, R"(LayerProperties { HiddenLayers { 0 4 5 9 } })");
    loadAndExpectLayersToBeRestored(mapFilePath.string(), { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    EXPECT_EQ(GlobalMapModule().getRoot()->getLayerManager().getParentLayer(3), 9) << "Failed to restore hierarchy";
    expectLayersAreVisible({ 1,2,3,6,7,8 }, true);
    expectLayersAreVisible({ 0,4,5,9 }, false);
}

}
