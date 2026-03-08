#include "RadiantTest.h"

#include "iselection.h"
#include "iselectiongroup.h"
#include "icommandsystem.h"
#include "scene/EntityNode.h"
#include "selectionlib.h"
#include "scenelib.h"
#include "algorithm/Entity.h"

namespace test
{

using SelectionAlgorithmTest = RadiantTest;

TEST_F(RadiantTest, SelectItemsByModel)
{
    loadMap("select_items_by_model.map");

    auto staticMeshPath = "models/just_a_static_mesh.ase";
    auto md5MeshPath = "just_an_md5.md5mesh";

    GlobalSelectionSystem().setSelectedAll(false);
    ASSERT_TRUE(GlobalSelectionSystem().getSelectionInfo().totalCount == 0);

    // Select the two entities with the regular "model" "" spawnarg
    GlobalCommandSystem().executeCommand("SelectItemsByModel", { staticMeshPath });
    ASSERT_TRUE(GlobalSelectionSystem().getSelectionInfo().entityCount == 2);

    // Deselect the two entities with the regular "model" "" spawnarg
    GlobalCommandSystem().executeCommand("DeselectItemsByModel", { staticMeshPath });
    ASSERT_TRUE(GlobalSelectionSystem().getSelectionInfo().totalCount == 0);

    // Select the three entities that reference the md5 mesh through a model def
    GlobalCommandSystem().executeCommand("SelectItemsByModel", { md5MeshPath });
    ASSERT_TRUE(GlobalSelectionSystem().getSelectionInfo().entityCount == 3);

    GlobalCommandSystem().executeCommand("DeselectItemsByModel", { md5MeshPath });
    ASSERT_TRUE(GlobalSelectionSystem().getSelectionInfo().totalCount == 0);
}

TEST_F(SelectionAlgorithmTest, GroupCycleThroughSelectionGroup)
{
    auto entity1 = algorithm::createEntityByClassName("fixed_size_entity");
    auto entity2 = algorithm::createEntityByClassName("fixed_size_entity");
    auto entity3 = algorithm::createEntityByClassName("fixed_size_entity");

    GlobalMapModule().getRoot()->addChildNode(entity1);
    GlobalMapModule().getRoot()->addChildNode(entity2);
    GlobalMapModule().getRoot()->addChildNode(entity3);

    auto& groupMgr = GlobalMapModule().getRoot()->getSelectionGroupManager();
    auto group = groupMgr.createSelectionGroup();
    group->addNode(entity1);
    group->addNode(entity2);
    group->addNode(entity3);

    Node_setSelected(entity1, true);
    Node_setSelected(entity2, true);
    Node_setSelected(entity3, true);

    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 3);

    GlobalCommandSystem().executeCommand("GroupCycleForward");
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1)
        << "After cycling forward, only one group member should be selected";

    auto firstSelected = GlobalSelectionSystem().ultimateSelected();
    GlobalCommandSystem().executeCommand("GroupCycleForward");
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1);

    auto secondSelected = GlobalSelectionSystem().ultimateSelected();
    EXPECT_NE(firstSelected, secondSelected)
        << "Cycling forward should select a different group member";

    GlobalCommandSystem().executeCommand("GroupCycleBackward");
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1);

    auto thirdSelected = GlobalSelectionSystem().ultimateSelected();
    EXPECT_EQ(thirdSelected, firstSelected)
        << "Cycling backward should return to the previously selected member";
}

}
