#include "RadiantTest.h"

#include <set>
#include "imap.h"
#include "ipatch.h"
#include "iselectable.h"
#include "itexturetoolmodel.h"
#include "iselection.h"
#include "scenelib.h"
#include "algorithm/Primitives.h"

namespace test
{

using TextureToolTest = RadiantTest;

inline std::size_t getTextureToolNodeCount()
{
    std::size_t count = 0;

    GlobalTextureToolSceneGraph().foreachNode([&](const textool::INode::Ptr& node)
    {
        ++count;
        return true;
    });

    return count;
}

inline textool::INode::Ptr getFirstTextureToolNode()
{
    textool::INode::Ptr returnValue;

    GlobalTextureToolSceneGraph().foreachNode([&](const textool::INode::Ptr& node)
    {
        returnValue = node;
        return false;
    });

    return returnValue;
}

// Checks that changing the regular scene selection will have an effect on the tex tool scene
TEST_F(TextureToolTest, SceneGraphObservesSelection)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::createCubicBrush(worldspawn, Vector3(0,0,0), "textures/numbers/1");
    auto brush2 = algorithm::createCubicBrush(worldspawn, Vector3(0,256,256), "textures/numbers/1");

    // Empty tex tool scenegraph on empty scene selection
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0) << "Non-empty selection at startup";
    EXPECT_EQ(getTextureToolNodeCount(), 0) << "There shouldn't be any textool nodes when the scene is empty";

    Node_setSelected(brush1, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "1 Brush must be selected";

    // We don't know how many tex tool nodes there are, but it should be more than 0
    auto nodeCount = getTextureToolNodeCount();
    EXPECT_GT(nodeCount, 0) << "There should be some tex tool nodes now";

    Node_setSelected(brush2, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 2) << "2 Brushes must be selected";

    // Should be even more now
    auto nodeCount2 = getTextureToolNodeCount();
    EXPECT_GT(nodeCount2, nodeCount) << "There should be more tex tool nodes now";

    GlobalSelectionSystem().setSelectedAll(false);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0) << "Non-empty selection at shutdown";
    EXPECT_EQ(getTextureToolNodeCount(), 0) << "There shouldn't be any textool nodes when the scene is empty";
}

TEST_F(TextureToolTest, SceneGraphNeedsUniqueShader)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::createCubicBrush(worldspawn, Vector3(0, 0, 0), "textures/numbers/1");
    auto brush2 = algorithm::createCubicBrush(worldspawn, Vector3(0, 256, 256), "textures/numbers/2");

    Node_setSelected(brush1, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "1 Brush must be selected";

    // We don't know how many tex tool nodes there are, but it should be more than 0
    EXPECT_GT(getTextureToolNodeCount(), 0) << "There should be some tex tool nodes now";

    Node_setSelected(brush2, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 2) << "2 Brushes must be selected";

    EXPECT_EQ(getTextureToolNodeCount(), 0) << "There should be no nodes now, since the material is non unique";

    // Deselect brush 1, now only brush 2 is selected
    Node_setSelected(brush1, false);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "1 Brush must be selected";
    EXPECT_GT(getTextureToolNodeCount(), 0) << "There should be some tex tool nodes again";
}

TEST_F(TextureToolTest, SceneGraphRecognisesBrushes)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::createCubicBrush(worldspawn, Vector3(0, 0, 0), "textures/numbers/1");

    Node_setSelected(brush1, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "1 Brush must be selected";

    // We don't know how many tex tool nodes there are, but it should be more than 0
    EXPECT_GT(getTextureToolNodeCount(), 0) << "There should be some tex tool nodes now";
}

TEST_F(TextureToolTest, SceneGraphRecognisesPatches)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto patch = GlobalPatchModule().createPatch(patch::PatchDefType::Def2);
    scene::addNodeToContainer(patch, worldspawn);

    Node_setSelected(patch, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "1 patch must be selected";

    // We don't know how many tex tool nodes there are, but it should be more than 0
    EXPECT_GT(getTextureToolNodeCount(), 0) << "There should be some tex tool nodes now";
}

TEST_F(TextureToolTest, PatchNodeBounds)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto patchNode = GlobalPatchModule().createPatch(patch::PatchDefType::Def2);

    auto patch = Node_getIPatch(patchNode);
    patch->setDims(3, 3);

    auto origin = Vector2(5.4, -78.3);
    auto step = 0.45;
    // Accumulate the test bounds
    AABB checkedBounds;

    for (auto col = 0; col < 3; ++col)
    {
        for (auto row = 0; row < 3; ++row)
        {
            auto& ctrl = patch->ctrlAt(row, col);

            ctrl.texcoord = Vector2(origin.x() + step * col, origin.y() + step * row);
            checkedBounds.includePoint({ ctrl.texcoord.x(), ctrl.texcoord.y(), 0 });
        }
    }

    scene::addNodeToContainer(patchNode, worldspawn);
    Node_setSelected(patchNode, true);

    auto node = getFirstTextureToolNode();
    EXPECT_TRUE(node) << "No texture tool node here";

    EXPECT_TRUE(math::isNear(node->localAABB().getOrigin(), checkedBounds.getOrigin(), 0.01)) << 
        "Bounds mismatch, got " << node->localAABB().getOrigin() << " instead of " << checkedBounds.getOrigin();
    EXPECT_TRUE(math::isNear(node->localAABB().getExtents(), checkedBounds.getExtents(), 0.01)) <<
        "Bounds mismatch, got " << node->localAABB().getExtents() << " instead of " << checkedBounds.getExtents();
}

TEST_F(TextureToolTest, ForeachSelectedNode)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush1 = algorithm::createCubicBrush(worldspawn, Vector3(0, 0, 0), "textures/numbers/1");
    auto brush2 = algorithm::createCubicBrush(worldspawn, Vector3(0, 256, 256), "textures/numbers/1");
    auto patchNode = GlobalPatchModule().createPatch(patch::PatchDefType::Def2);
    Node_getIPatch(patchNode)->setDims(3, 3);
    Node_getIPatch(patchNode)->setShader("textures/numbers/1");

    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(patchNode, true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 3) << "3 items must be selected";

    // We don't know how many tex tool nodes there are, but it should be more than 0
    EXPECT_GT(getTextureToolNodeCount(), 0) << "There should be some tex tool nodes now";

    std::set<textool::INode::Ptr> selectedNodes;
    std::size_t i = 0;

    // Selected every odd node
    GlobalTextureToolSceneGraph().foreachNode([&](const textool::INode::Ptr& node)
    {
        if (++i % 2 == 1)
        {
            node->setSelected(true);
            selectedNodes.emplace(node);
        }

        return true;
    });

    std::size_t selectedCount = 0;
    GlobalTextureToolSelectionSystem().foreachSelectedNode([&](const textool::INode::Ptr& node)
    {
        ++selectedCount;
        EXPECT_TRUE(selectedNodes.count(node) > 0) << "Node shouldn't be selected";
        return true;
    });

    EXPECT_EQ(selectedCount, selectedNodes.size()) << "Selection count didn't match";
}

}
