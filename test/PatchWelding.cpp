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

namespace
{

void performPatchWeldingTest(const std::string& number1, const std::string& number2, int expectedRows, int expectedCols)
{
    auto firstPatch = findPatchWithNumber(number1);
    auto secondPatch = findPatchWithNumber(number2);

    Node_setSelected(firstPatch, true);
    Node_setSelected(secondPatch, true);

    GlobalCommandSystem().executeCommand("WeldSelectedPatches");

    // Only one patch selected
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1);

    // Two patches removed from the scene
    EXPECT_FALSE(firstPatch->getParent());
    EXPECT_FALSE(secondPatch->getParent());

    auto merged = std::dynamic_pointer_cast<IPatchNode>(GlobalSelectionSystem().ultimateSelected());
    EXPECT_EQ(merged->getPatch().getHeight(), expectedRows);
    EXPECT_EQ(merged->getPatch().getWidth(), expectedCols);
}

}

// Patch 1 = 3x3, Patch 4 = 3x3 to the right
TEST_F(PatchWeldingTest, WeldPatches1And4)
{
    loadMap("weld_patches.mapx");

    // Welding patches 1 and 4 should produce a 3 x 5 patch
    performPatchWeldingTest("1", "4", 3, 5);
}

}
