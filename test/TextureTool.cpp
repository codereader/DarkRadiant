#include "RadiantTest.h"

#include "imap.h"
#include "iselectable.h"
#include "itexturetoolmodel.h"
#include "iselection.h"
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

}
