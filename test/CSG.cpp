#include "RadiantTest.h"

#include "imap.h"
#include "ibrush.h"
#include "entitylib.h"
#include "algorithm/Scene.h"

namespace test
{

TEST_F(RadiantTest, CSGMergeTwoRegularWorldspawnBrushes)
{
    loadMap("csg_merge.map");

    // Locate the first worldspawn brush
    auto worldspawn = GlobalMapModule().getWorldspawn();

    // Try to merge the two brushes with the "1" and "2" materials
    auto firstBrush = algorithm::findFirstBrushWithMaterial(worldspawn, "1");
    auto secondBrush = algorithm::findFirstBrushWithMaterial(worldspawn, "2");

    ASSERT_TRUE(Node_getIBrush(firstBrush)->getNumFaces() == 5);
    ASSERT_TRUE(Node_getIBrush(secondBrush)->getNumFaces() == 5);

    // Select the brushes and merge them
    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(firstBrush, true);
    Node_setSelected(secondBrush, true);

    // CSG merge
    GlobalCommandSystem().executeCommand("CSGMerge");

    // The two brushes should be gone, replaced by a new one
    ASSERT_TRUE(firstBrush->getParent() == nullptr);
    ASSERT_TRUE(secondBrush->getParent() == nullptr);

    // The merged brush will carry both materials
    auto brushWithMaterial1 = algorithm::findFirstBrushWithMaterial(worldspawn, "1");
    auto brushWithMaterial2 = algorithm::findFirstBrushWithMaterial(worldspawn, "2");

    ASSERT_TRUE(brushWithMaterial1 == brushWithMaterial2);
    ASSERT_TRUE(Node_getIBrush(brushWithMaterial1)->getNumFaces() == 6);
}

TEST_F(RadiantTest, CSGMergeFourRegularWorldspawnBrushes)
{
    loadMap("csg_merge.map");

    // Locate the first worldspawn brush
    auto worldspawn = GlobalMapModule().getWorldspawn();

    // Try to merge the two brushes with the "1" and "2" materials
    std::vector<scene::INodePtr> brushes = {
        algorithm::findFirstBrushWithMaterial(worldspawn, "1"),
        algorithm::findFirstBrushWithMaterial(worldspawn, "2"),
        algorithm::findFirstBrushWithMaterial(worldspawn, "3"),
        algorithm::findFirstBrushWithMaterial(worldspawn, "4")
    };

    // Check the correct setup
    for (const auto& brush : brushes)
    {
        ASSERT_TRUE(Node_getIBrush(brush)->getNumFaces() == 5);
    }

    // Select the brushes and merge them
    GlobalSelectionSystem().setSelectedAll(false);
    for (const auto& brush : brushes)
    {
        Node_setSelected(brush, true);
    }

    // CSG merge
    GlobalCommandSystem().executeCommand("CSGMerge");

    // All brushes should be gone, replaced by a new one
    for (const auto& brush : brushes)
    {
        ASSERT_TRUE(brush->getParent() == nullptr);
    }

    // The combined brush should be a 6-sided cuboid
    auto brushWithMaterial1 = algorithm::findFirstBrushWithMaterial(worldspawn, "1");
    ASSERT_TRUE(Node_getIBrush(brushWithMaterial1)->getNumFaces() == 6);
}

TEST_F(RadiantTest, CSGMergeTwoFuncStaticBrushes)
{
    loadMap("csg_merge.map");

    // Locate the func_static in the map
    EntityNodeFindByClassnameWalker walker("func_static");
    GlobalSceneGraph().root()->traverse(walker);

    auto entity = walker.getEntityNode();

    // Try to merge the two brushes with the "1" and "2" materials
    auto firstBrush = algorithm::findFirstBrushWithMaterial(entity, "1");
    auto secondBrush = algorithm::findFirstBrushWithMaterial(entity, "2");

    ASSERT_TRUE(Node_getIBrush(firstBrush)->getNumFaces() == 5);
    ASSERT_TRUE(Node_getIBrush(secondBrush)->getNumFaces() == 5);

    // Select the brushes and merge them
    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(firstBrush, true);
    Node_setSelected(secondBrush, true);

    // CSG merge
    GlobalCommandSystem().executeCommand("CSGMerge");

    // The two brushes should be gone, replaced by a new one
    ASSERT_TRUE(firstBrush->getParent() == nullptr);
    ASSERT_TRUE(secondBrush->getParent() == nullptr);

    // The merged brush will carry both materials
    auto brushWithMaterial1 = algorithm::findFirstBrushWithMaterial(entity, "1");
    auto brushWithMaterial2 = algorithm::findFirstBrushWithMaterial(entity, "2");

    ASSERT_TRUE(brushWithMaterial1 == brushWithMaterial2);
    ASSERT_TRUE(Node_getIBrush(brushWithMaterial1)->getNumFaces() == 6);

    // They should still be children of the same entity
    ASSERT_TRUE(brushWithMaterial1->getParent() == entity);
    ASSERT_TRUE(brushWithMaterial2->getParent() == entity);
}

// #5344: Check that selecting a couple of brushes will only merge those
// which share the same parent entity
TEST_F(RadiantTest, CSGMergeBrushesOfMixedEntitySelection)
{
    loadMap("csg_merge.map");

    // Locate the func_static in the map
    EntityNodeFindByClassnameWalker walker("func_static");
    GlobalSceneGraph().root()->traverse(walker);

    auto entity = walker.getEntityNode();
    auto worldspawn = GlobalMapModule().getWorldspawn();

    // Select the mergeable brushes of both entities carrying the "1" and "2" materials
    std::vector<scene::INodePtr> brushes = {
        algorithm::findFirstBrushWithMaterial(entity, "1"),
        algorithm::findFirstBrushWithMaterial(entity, "2"),
        algorithm::findFirstBrushWithMaterial(worldspawn, "1"),
        algorithm::findFirstBrushWithMaterial(worldspawn, "2")
    };

    // Check the correct setup
    for (const auto& brush : brushes)
    {
        ASSERT_TRUE(Node_getIBrush(brush)->getNumFaces() == 5);
    }

    // Select the brushes and merge them
    GlobalSelectionSystem().setSelectedAll(false);

    for (const auto& brush : brushes)
    {
        Node_setSelected(brush, true);
    }

    // CSG merge
    GlobalCommandSystem().executeCommand("CSGMerge");

    // All brushes should be gone, replaced by TWO new ones
    for (const auto& brush : brushes)
    {
        ASSERT_TRUE(brush->getParent() == nullptr);
    }

    // The merged brush will carry both materials
    auto funcBrush1 = algorithm::findFirstBrushWithMaterial(entity, "1");
    auto funcBrush2 = algorithm::findFirstBrushWithMaterial(entity, "2");

    ASSERT_TRUE(funcBrush1);
    ASSERT_TRUE(funcBrush1 == funcBrush2);
    ASSERT_TRUE(Node_getIBrush(funcBrush1)->getNumFaces() == 6);

    // Same for the worldspawn entity
    auto worldBrush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "1");
    auto worldBrush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "2");

    ASSERT_TRUE(worldBrush1);
    ASSERT_TRUE(worldBrush1 == worldBrush2);
    ASSERT_TRUE(Node_getIBrush(worldBrush1)->getNumFaces() == 6);
}

// Issue #5336: Crash when using CSG Merge on brushes that are part of worldspawn and a func_static
TEST_F(RadiantTest, CSGMergeWithFuncStatic)
{
    loadMap("csg_merge_with_func_static.map");

    // Locate the first worldspawn brush
    auto firstBrush = algorithm::getNthChild(GlobalMapModule().getWorldspawn(), 0);
    ASSERT_TRUE(firstBrush);

    // Locate the func_static in the map
    EntityNodeFindByClassnameWalker walker("func_static");
    GlobalSceneGraph().root()->traverse(walker);

    auto entityNode = walker.getEntityNode();
    ASSERT_TRUE(entityNode);
    ASSERT_TRUE(entityNode->hasChildNodes());

    // Select both of them, the order is important
    Node_setSelected(firstBrush, true);
    Node_setSelected(entityNode, true);

    // CSG merge
    GlobalCommandSystem().executeCommand("CSGMerge");

    // No merge should have happened since the brushes 
    // are not part of the same entity
    // So assume the scene didn't change
    ASSERT_TRUE(algorithm::getNthChild(GlobalMapModule().getWorldspawn(), 0) == firstBrush);

    EntityNodeFindByClassnameWalker walker2("func_static");
    GlobalSceneGraph().root()->traverse(walker2);

    ASSERT_TRUE(walker.getEntityNode());
    ASSERT_TRUE(walker.getEntityNode()->hasChildNodes());
}

}
