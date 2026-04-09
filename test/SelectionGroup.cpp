#include "RadiantTest.h"

#include "iselection.h"
#include "iselectiongroup.h"
#include "imap.h"
#include "selectionlib.h"
#include "algorithm/Scene.h"
#include "scene/Group.h"
#include "command/ExecutionNotPossible.h"

namespace test
{

using SelectionGroupTest = RadiantTest;

TEST_F(SelectionGroupTest, GroupSelectedNodes)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");

    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);

    selection::groupSelected();

    auto selectable1 = std::dynamic_pointer_cast<IGroupSelectable>(brush1);
    auto selectable2 = std::dynamic_pointer_cast<IGroupSelectable>(brush2);

    EXPECT_TRUE(selectable1->isGroupMember());
    EXPECT_TRUE(selectable2->isGroupMember());
    EXPECT_EQ(selectable1->getMostRecentGroupId(), selectable2->getMostRecentGroupId());
}

TEST_F(SelectionGroupTest, UngroupSelectedNodes)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");

    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    selection::groupSelected();

    GlobalSelectionSystem().setSelectedAll(false);

    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    selection::ungroupSelected();

    auto selectable1 = std::dynamic_pointer_cast<IGroupSelectable>(brush1);
    auto selectable2 = std::dynamic_pointer_cast<IGroupSelectable>(brush2);

    EXPECT_FALSE(selectable1->isGroupMember());
    EXPECT_FALSE(selectable2->isGroupMember());
}

TEST_F(SelectionGroupTest, UngroupOnlyRemovesMostRecentGroup)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");
    auto brush3 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/3");

    // Create inner group
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    selection::groupSelected();
    GlobalSelectionSystem().setSelectedAll(false);

    // Create outer group
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(brush3, true);
    selection::groupSelected();
    GlobalSelectionSystem().setSelectedAll(false);

    auto selectable1 = std::dynamic_pointer_cast<IGroupSelectable>(brush1);
    EXPECT_EQ(selectable1->getGroupIds().size(), 2);

    // Ungroup should only remove the outer group
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(brush3, true);
    selection::ungroupSelected();

    EXPECT_TRUE(selectable1->isGroupMember()) << "Inner group should still exist";
    EXPECT_EQ(selectable1->getGroupIds().size(), 1);
}

TEST_F(SelectionGroupTest, UngroupRecursivelyRemovesAllGroups)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");
    auto brush3 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/3");

    // Create inner group
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    selection::groupSelected();
    GlobalSelectionSystem().setSelectedAll(false);

    // Create outer group
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(brush3, true);
    selection::groupSelected();
    GlobalSelectionSystem().setSelectedAll(false);

    auto selectable1 = std::dynamic_pointer_cast<IGroupSelectable>(brush1);
    auto selectable2 = std::dynamic_pointer_cast<IGroupSelectable>(brush2);
    auto selectable3 = std::dynamic_pointer_cast<IGroupSelectable>(brush3);

    EXPECT_EQ(selectable1->getGroupIds().size(), 2);
    EXPECT_EQ(selectable2->getGroupIds().size(), 2);
    EXPECT_EQ(selectable3->getGroupIds().size(), 1);

    // Recursive ungroup should remove all groups
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(brush3, true);
    selection::ungroupSelectedRecursively();

    EXPECT_FALSE(selectable1->isGroupMember());
    EXPECT_FALSE(selectable2->isGroupMember());
    EXPECT_FALSE(selectable3->isGroupMember());
    EXPECT_EQ(selectable1->getGroupIds().size(), 0);
    EXPECT_EQ(selectable2->getGroupIds().size(), 0);
    EXPECT_EQ(selectable3->getGroupIds().size(), 0);
}

TEST_F(SelectionGroupTest, UngroupRecursivelyWithThreeLevels)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");
    auto brush3 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/3");

    // Level 1
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    selection::groupSelected();
    GlobalSelectionSystem().setSelectedAll(false);

    // Level 2
    Node_setSelected(brush2, true);
    Node_setSelected(brush3, true);
    selection::groupSelected();
    GlobalSelectionSystem().setSelectedAll(false);

    // Level 3
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(brush3, true);
    selection::groupSelected();
    GlobalSelectionSystem().setSelectedAll(false);

    auto selectable1 = std::dynamic_pointer_cast<IGroupSelectable>(brush1);
    auto selectable2 = std::dynamic_pointer_cast<IGroupSelectable>(brush2);
    auto selectable3 = std::dynamic_pointer_cast<IGroupSelectable>(brush3);
    EXPECT_EQ(selectable1->getGroupIds().size(), 2);
    EXPECT_EQ(selectable2->getGroupIds().size(), 3);

    // Recursive ungroup on all three
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(brush3, true);
    selection::ungroupSelectedRecursively();

    EXPECT_FALSE(selectable1->isGroupMember());
    EXPECT_FALSE(selectable2->isGroupMember());
    EXPECT_FALSE(selectable3->isGroupMember());
}

TEST_F(SelectionGroupTest, UngroupRecursivelyOnSubsetAffectsSharedGroups)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");
    auto brush3 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/3");

    // Create group
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(brush3, true);
    selection::groupSelected();
    GlobalSelectionSystem().setSelectedAll(false);

    // Recursive ungroup with only brush1 selected
    // This should delete the group, which also removes brush2 and brush3 from it
    Node_setSelected(brush1, true);
    selection::ungroupSelectedRecursively();

    auto selectable1 = std::dynamic_pointer_cast<IGroupSelectable>(brush1);
    auto selectable2 = std::dynamic_pointer_cast<IGroupSelectable>(brush2);
    auto selectable3 = std::dynamic_pointer_cast<IGroupSelectable>(brush3);

    EXPECT_FALSE(selectable1->isGroupMember());
    EXPECT_FALSE(selectable2->isGroupMember());
    EXPECT_FALSE(selectable3->isGroupMember());
}

TEST_F(SelectionGroupTest, UngroupRecursivelyThrowsWithNothingSelected)
{
    GlobalSelectionSystem().setSelectedAll(false);

    EXPECT_THROW(selection::ungroupSelectedRecursively(), cmd::ExecutionNotPossible);
}

TEST_F(SelectionGroupTest, UngroupRecursivelyThrowsWithNoGroups)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");

    Node_setSelected(brush1, true);

    EXPECT_THROW(selection::ungroupSelectedRecursively(), cmd::ExecutionNotPossible);
}

TEST_F(SelectionGroupTest, RemoveNodeFromGroup)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");
    auto brush3 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/3");

    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(brush3, true);
    selection::groupSelected();
    GlobalSelectionSystem().setSelectedAll(false);

    auto selectable1 = std::dynamic_pointer_cast<IGroupSelectable>(brush1);
    auto selectable2 = std::dynamic_pointer_cast<IGroupSelectable>(brush2);
    auto selectable3 = std::dynamic_pointer_cast<IGroupSelectable>(brush3);

    auto groupId = selectable1->getMostRecentGroupId();
    auto& mgr = GlobalMapModule().getRoot()->getSelectionGroupManager();
    auto group = mgr.getSelectionGroup(groupId);

    EXPECT_EQ(group->size(), 3);

    // Remove one node from the group
    group->removeNode(brush2);

    EXPECT_TRUE(selectable1->isGroupMember());
    EXPECT_FALSE(selectable2->isGroupMember());
    EXPECT_TRUE(selectable3->isGroupMember());
    EXPECT_EQ(group->size(), 2);
}

}
