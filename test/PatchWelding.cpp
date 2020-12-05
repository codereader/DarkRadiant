#include "RadiantTest.h"

#include "icommandsystem.h"
#include "iselection.h"
#include "ipatch.h"
#include "algorithm/Scene.h"
#include "scenelib.h"

namespace test
{

using PatchWeldingTest = RadiantTest;

inline scene::INodePtr findPatchWithNumber(const std::string& number)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    return algorithm::findFirstPatchWithMaterial(worldspawn, "textures/numbers/" + number);
}

// Patch 1 = 3x3, Patch 4 = 3x3 to the right
TEST_F(PatchWeldingTest, WeldPatches1And4)
{
    loadMap("weld_patches.mapx");

    auto patch1 = findPatchWithNumber("1");
    auto patch4 = findPatchWithNumber("4");

    Node_setSelected(patch1, true);
    Node_setSelected(patch4, true);

    GlobalCommandSystem().executeCommand("WeldSelectedPatches");

    // Only one patch selected
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1);

    // Two patches removed from the scene
    EXPECT_FALSE(patch1->getParent());
    EXPECT_FALSE(patch4->getParent());

    auto merged = std::dynamic_pointer_cast<IPatchNode>(GlobalSelectionSystem().ultimateSelected());
    EXPECT_EQ(merged->getPatch().getWidth(), 3);
    EXPECT_EQ(merged->getPatch().getWidth(), 5);
}

}
