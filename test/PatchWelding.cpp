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
    return algorithm::findFirstPatchWithMaterial(GlobalMapModule().getRoot(), "textures/numbers/" + number);
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

void assumePatchWeldingFails(const std::string& number1, const std::string& number2)
{
    auto firstPatch = findPatchWithNumber(number1);
    auto secondPatch = findPatchWithNumber(number2);

    Node_setSelected(firstPatch, true);
    Node_setSelected(secondPatch, true);

    GlobalCommandSystem().executeCommand("WeldSelectedPatches");

    // Still two patches selected after this operation
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 2);

    EXPECT_TRUE(Node_isSelected(firstPatch));
    EXPECT_TRUE(Node_isSelected(secondPatch));

    // Two patches not removed from the scene
    EXPECT_TRUE(firstPatch->getParent());
    EXPECT_TRUE(secondPatch->getParent());
}

}

// Patch 1 = 3x3, Patch 2 = 3x3 to the "left"
TEST_F(PatchWeldingTest, WeldPatches1And2)
{
    loadMap("weld_patches.mapx");

    // Welding patches 1 and 2 should produce a 3rows x 5cols patch
    performPatchWeldingTest("1", "2", 3, 5);
}

// Patch 1 = 3x3, Patch 4 = 3x3 to the "right"
TEST_F(PatchWeldingTest, WeldPatches1And4)
{
    loadMap("weld_patches.mapx");

    // Welding patches 1 and 4 should produce a 3rows x 5cols patch
    performPatchWeldingTest("1", "4", 3, 5);
}

// Patch 1 = 3x3, Patch 3 = 3x3 adjacent at the "top"
TEST_F(PatchWeldingTest, WeldPatches1And3)
{
    loadMap("weld_patches.mapx");

    // Welding patches 1 and 3 should produce a 5rows x 3cols patch
    performPatchWeldingTest("1", "3", 5, 3);
}

// Patch 1 = 3x3, Patch 5 = 3x3 adjacent at the "bottom"
TEST_F(PatchWeldingTest, WeldPatches1And5)
{
    loadMap("weld_patches.mapx");

    // Welding patches 1 and 5 should produce a 5rows x 3cols patch
    performPatchWeldingTest("1", "5", 5, 3);
}

// Patch 6 = 3rows x 5cols, Patch 8 = 3rows x 5cols adjacent to the right, both are func_static children
TEST_F(PatchWeldingTest, WeldPatches6And8)
{
    loadMap("weld_patches.mapx");

    // Welding patches 6 and 8 should produce a 3rows x 9cols patch
    performPatchWeldingTest("6", "8", 3, 9);
}

// Patch 6 = 3rows x 5cols, Patch 9 = 3rows x 5cols adjacent at the top, both are func_static children
TEST_F(PatchWeldingTest, WeldPatches6And9)
{
    loadMap("weld_patches.mapx");

    // Welding patches 6 and 9 should produce a 5rows x 5cols patch
    performPatchWeldingTest("6", "9", 5, 5);
}

// Patch 6 = 3rows x 5cols, Patch 10 = 5rows x 3cols adjacent at the bottom, both are func_static children
TEST_F(PatchWeldingTest, WeldPatches6And10)
{
    loadMap("weld_patches.mapx");

    // Welding patches 6 and 10 should produce a 5rows x 5cols patch
    performPatchWeldingTest("6", "10", 5, 5);
}

// Patch 6 = 3rows x 5cols, Patch 7 = 3rows x 3cols adjacent at the bottom, both are func_static children
TEST_F(PatchWeldingTest, WeldPatches6And7)
{
    loadMap("weld_patches.mapx");

    // Welding patches 6 and 7 should produce a 3rows x 7cols patch
    performPatchWeldingTest("6", "7", 3, 7);
}

// Patch 7 and Patch 8 are not adjacent
TEST_F(PatchWeldingTest, WeldPatches7And8)
{
    loadMap("weld_patches.mapx");

    // Patches have the same parent, but are not adjacent
    assumePatchWeldingFails("7", "8");
}

// Patch 4 = worldspawn, Patch 7 = func_static 
TEST_F(PatchWeldingTest, WeldPatches4And7)
{
    loadMap("weld_patches.mapx");

    // Welding patches 4 and 7 fails because they belong to different entities
    assumePatchWeldingFails("4", "7");
}

// Patches 11 are 12 are two cylinders stack upon each other
TEST_F(PatchWeldingTest, WeldPatches11And12)
{
    loadMap("weld_patches.mapx");

    // Welding the two cylinders produce a 5rows x 9cols patch
    performPatchWeldingTest("11", "12", 5, 9);
}

}
