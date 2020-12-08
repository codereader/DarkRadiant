#include "RadiantTest.h"

#include "icommandsystem.h"
#include "iselection.h"
#include "ipatch.h"
#include "algorithm/Scene.h"
#include "scenelib.h"

namespace test
{

using PatchWeldingTest = RadiantTest;

class PatchWelding3x3 :
    public PatchWeldingTest,
    public testing::WithParamInterface<std::tuple<const char*, const char*, int, int>>
{};

namespace
{

inline scene::INodePtr findPatchWithNumber(const std::string& number)
{
    return algorithm::findFirstPatchWithMaterial(GlobalMapModule().getRoot(), "textures/numbers/" + number);
}

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

TEST_P(PatchWelding3x3, WeldWithOther3x3Patch)
{
    loadMap("weld_patches2.mapx");

    auto firstPatch = std::get<0>(GetParam());
    auto secondPatch = std::get<1>(GetParam());
    auto expectedRows = std::get<2>(GetParam());
    auto expectedColumns = std::get<3>(GetParam());

    performPatchWeldingTest(firstPatch, secondPatch, expectedRows, expectedColumns);
}

// Patch 1 is sharing its first row
INSTANTIATE_TEST_CASE_P(WeldPatch1WithOther3x3, PatchWelding3x3, 
    testing::Values(std::tuple{ "1", "2", 5, 3 }, 
                    std::tuple{ "1", "3", 5, 3 }, 
                    std::tuple{ "1", "4", 5, 3 }, 
                    std::tuple{ "1", "5", 5, 3 },
                    std::tuple{ "2", "1", 1, 1 }, // TODO
                    std::tuple{ "3", "1", 1, 1 }, // TODO
                    std::tuple{ "4", "1", 1, 1 }, // TODO
                    std::tuple{ "5", "1", 1, 1 })); // TODO

// Patch 6 is sharing its last row
INSTANTIATE_TEST_CASE_P(WeldPatch6WithOther3x3, PatchWelding3x3,
    testing::Values(std::tuple{ "6", "7", 5, 3 }, 
                    std::tuple{ "6", "8", 5, 3 }, 
                    std::tuple{ "6", "9", 5, 3 }, 
                    std::tuple{ "6", "10", 5, 3 },
                    std::tuple{ "7", "6", 1, 1 }, // TODO
                    std::tuple{ "8", "6", 1, 1 }, // TODO
                    std::tuple{ "9", "6", 1, 1 }, // TODO
                    std::tuple{ "10", "6", 1, 1 })); // TODO

// Patch 11 is sharing a column
INSTANTIATE_TEST_CASE_P(WeldPatch11WithOther3x3, PatchWelding3x3,
    testing::Values(std::tuple{ "11", "12", 3, 5 }, 
                    std::tuple{ "11", "13", 3, 5 }, 
                    std::tuple{ "11", "14", 3, 5 }, 
                    std::tuple{ "11", "15", 3, 5 },
                    std::tuple{ "12", "11", 1, 1 }, // TODO
                    std::tuple{ "13", "11", 1, 1 }, // TODO
                    std::tuple{ "14", "11", 1, 1 }, // TODO
                    std::tuple{ "15", "11", 1, 1 })); // TODO

// Patch 16 is sharing a column
INSTANTIATE_TEST_CASE_P(WeldPatch16WithOther3x3, PatchWelding3x3,
    testing::Values(std::tuple{ "16", "17", 3, 5 }, 
                    std::tuple{ "16", "18", 3, 5 }, 
                    std::tuple{ "16", "19", 3, 5 }, 
                    std::tuple{ "16", "20", 3, 5 },
                    std::tuple{ "17", "16", 1, 1 }, // TODO
                    std::tuple{ "18", "16", 1, 1 }, // TODO
                    std::tuple{ "19", "16", 1, 1 }, // TODO
                    std::tuple{ "20", "16", 1, 1 })); // TODO

#if 0
TEST_F(PatchWeldingTest, Weld3x3Patches1And5)
{
    loadMap("weld_patches2.mapx");

    // Welding should produce a 3rows x 5cols patch
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

// Patches 13 are 14 are matching at first row : first row
TEST_F(PatchWeldingTest, WeldPatches13And14)
{
    loadMap("weld_patches.mapx");

    // Welding the two cylinders produce a 5rows x 3cols patch
    performPatchWeldingTest("13", "14", 5, 3);
}
#endif
}
