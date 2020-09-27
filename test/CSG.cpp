#include "RadiantTest.h"

#include "imap.h"
#include "ibrush.h"
#include "entitylib.h"
#include "algorithm/Scene.h"

// Issue #5336: Crash when using CSG Merge on brushes that are entities
TEST_F(RadiantTest, CSGMergeWithFuncStatic)
{
    loadMap("csg_merge_with_func_static.map");

    // Locate the first worldspawn brush
    scene::INodePtr firstBrush = test::algorithm::getNthChild(GlobalMapModule().getWorldspawn(), 0);
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
    ASSERT_TRUE(test::algorithm::getNthChild(GlobalMapModule().getWorldspawn(), 0) == firstBrush);

    EntityNodeFindByClassnameWalker walker2("func_static");
    GlobalSceneGraph().root()->traverse(walker2);

    ASSERT_TRUE(walker.getEntityNode());
    ASSERT_TRUE(walker.getEntityNode()->hasChildNodes());
}
