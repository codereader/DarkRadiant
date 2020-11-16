#include "RadiantTest.h"

#include "ishaders.h"
#include "imap.h"
#include "ifilter.h"
#include "ibrush.h"
#include "ipatch.h"
#include "algorithm/Scene.h"
#include "scenelib.h"
#include "selectionlib.h"

namespace test
{

using SelectionTest = RadiantTest;

TEST_F(SelectionTest, ApplyShadersToForcedVisibleObjects)
{
    loadMap("primitives_with_clip_material.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/common/clip");
    auto patch = algorithm::findFirstPatchWithMaterial(worldspawn, "textures/common/clip");

    // Filter out the objects using the clip material
    GlobalFilterSystem().setFilterState("Clip Textures", true);

    // Glitch: Since this brush is completely filtered out, this won't be called by the filter system
    Node_getIBrush(brush)->updateFaceVisibility();

    EXPECT_FALSE(brush->visible());
    EXPECT_FALSE(patch->visible());

    Node_setSelected(brush, true);
    Node_setSelected(patch, true);

    EXPECT_TRUE(brush->visible());
    EXPECT_TRUE(patch->visible());

    // Apply the caulk material to the selection
    selection::applyShaderToSelection("textures/common/caulk");

    auto brushNode = std::dynamic_pointer_cast<IBrushNode>(brush);
    auto patchNode = std::dynamic_pointer_cast<IPatchNode>(patch);

    EXPECT_TRUE(brushNode->getIBrush().hasShader("textures/common/caulk"));
    EXPECT_EQ(patchNode->getPatch().getShader(), ("textures/common/caulk"));
}

}
