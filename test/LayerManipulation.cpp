#include "i18n.h"
#include "RadiantTest.h"

#include "imap.h"
#include "ilayer.h"
#include "algorithm/Scene.h"
#include "scenelib.h"

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

    layerManager.createLayer("TestLayer");
    layerManager.createLayer("TestLayer2");

    std::size_t layersChangedFireCount = 0;
    layerManager.signal_layersChanged().connect([&]()
    {
        ++layersChangedFireCount;
    });

    std::size_t layerVisibilityChangedFireCount = 0;
    layerManager.signal_layerVisibilityChanged().connect([&]()
    {
        ++layerVisibilityChangedFireCount;
    });

    layerManager.reset();

    EXPECT_EQ(layerManager.getLayerID(_("Default")), 0) << "We should have a default layer after reset";
    EXPECT_EQ(layerManager.getLayerName(0), _("Default")) << "We should have a default layer after reset";

    EXPECT_EQ(layerManager.getLayerID("TestLayer"), -1) << "The test layer should be gone now";
    EXPECT_EQ(layerManager.getLayerID("TestLayer2"), -1) << "The test layer should be gone now";

    EXPECT_EQ(layerManager.getActiveLayer(), 0) << "Default layer should be active";
    EXPECT_EQ(layerManager.layerIsVisible(0), true) << "Default layer should be visible";

    EXPECT_EQ(layersChangedFireCount, 1) << "Layers changed signal should have been fired";
    EXPECT_EQ(layerVisibilityChangedFireCount, 1) << "Layer visibility changed signal should have been fired";
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
