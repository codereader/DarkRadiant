#include "RadiantTest.h"

#include "imap.h"
#include "ilayer.h"
#include "algorithm/Scene.h"
#include "scenelib.h"

namespace test
{

using LayerTest = RadiantTest;

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
    EXPECT_NE(layerManager.getLayerID("Second Layer"), -1);

    layerManager.renameLayer(layerManager.getLayerID("Second Layer"), "Renamed Layer");

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
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

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
