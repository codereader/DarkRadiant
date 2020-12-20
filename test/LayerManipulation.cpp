#include "RadiantTest.h"

#include "imap.h"
#include "ilayer.h"
#include "algorithm/Scene.h"

namespace test
{

using LayerTest = RadiantTest;

TEST_F(LayerTest, CreateLayerMarksMapAsModified)
{
    loadMap("general_purpose.mapx");

    EXPECT_FALSE(GlobalMapModule().isModified());

    GlobalMapModule().getRoot()->getLayerManager().createLayer("Testlayer");

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
    EXPECT_NE(layerManager.getLayerID("Second Layer"), -1);

    layerManager.deleteLayer("Second Layer");

    EXPECT_TRUE(GlobalMapModule().isModified());
}

}
