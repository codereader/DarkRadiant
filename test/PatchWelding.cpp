#include "RadiantTest.h"

#include "icommandsystem.h"
#include "iselection.h"
#include "iselectiongroup.h"
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

scene::INodePtr performPatchWelding(const std::string& number1, const std::string& number2)
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

    return GlobalSelectionSystem().ultimateSelected();
}

void verifyPatchDimensions(const scene::INodePtr& mergedPatchNode, int expectedRows, int expectedCols)
{
    auto merged = std::dynamic_pointer_cast<IPatchNode>(mergedPatchNode);
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

    verifyPatchDimensions(performPatchWelding(firstPatch, secondPatch), expectedRows, expectedColumns);
}

// Patch 1 is sharing its first row
INSTANTIATE_TEST_CASE_P(WeldPatch1WithOther3x3, PatchWelding3x3, 
    testing::Values(std::tuple{ "1", "2", 5, 3 }, 
                    std::tuple{ "1", "3", 5, 3 }, 
                    std::tuple{ "1", "4", 5, 3 }, 
                    std::tuple{ "1", "5", 5, 3 },
                    std::tuple{ "2", "1", 5, 3 },
                    std::tuple{ "3", "1", 5, 3 },
                    std::tuple{ "4", "1", 5, 3 },
                    std::tuple{ "5", "1", 5, 3 }));

// Patch 6 is sharing its last row
INSTANTIATE_TEST_CASE_P(WeldPatch6WithOther3x3, PatchWelding3x3,
    testing::Values(std::tuple{ "6", "7", 5, 3 }, 
                    std::tuple{ "6", "8", 5, 3 }, 
                    std::tuple{ "6", "9", 5, 3 }, 
                    std::tuple{ "6", "10", 5, 3 },
                    std::tuple{ "7", "6", 5, 3 },
                    std::tuple{ "8", "6", 5, 3 },
                    std::tuple{ "9", "6", 5, 3 },
                    std::tuple{ "10", "6", 5, 3 }));

// Patch 11 is sharing a row
INSTANTIATE_TEST_CASE_P(WeldPatch11WithOther3x3, PatchWelding3x3,
    testing::Values(std::tuple{ "11", "12", 3, 5 }, 
                    std::tuple{ "11", "13", 3, 5 }, 
                    std::tuple{ "11", "14", 3, 5 }, 
                    std::tuple{ "11", "15", 3, 5 },
                    std::tuple{ "12", "11", 5, 3 },
                    std::tuple{ "13", "11", 5, 3 },
                    std::tuple{ "14", "11", 5, 3 },
                    std::tuple{ "15", "11", 5, 3 }));

// Patch 16 is sharing a column
INSTANTIATE_TEST_CASE_P(WeldPatch16WithOther3x3, PatchWelding3x3,
    testing::Values(std::tuple{ "16", "17", 3, 5 }, 
                    std::tuple{ "16", "18", 3, 5 }, 
                    std::tuple{ "16", "19", 3, 5 }, 
                    std::tuple{ "16", "20", 3, 5 },
                    std::tuple{ "17", "16", 5, 3 },
                    std::tuple{ "18", "16", 5, 3 },
                    std::tuple{ "19", "16", 5, 3 },
                    std::tuple{ "20", "16", 5, 3 }));

// Patch 4 = worldspawn, Patch 7 = func_static 
TEST_F(PatchWeldingTest, TryToWeldPatchesOfDifferentParents)
{
    loadMap("weld_patches.mapx");

    // Welding patches 4 and 7 fails because they belong to different entities
    assumePatchWeldingFails("4", "7");
}

// Patches 2 and 4 are not touching
TEST_F(PatchWeldingTest, TryToWeldNontouchingPatches)
{
    loadMap("weld_patches.mapx");

    assumePatchWeldingFails("4", "7");
}

// Patches 11 are 12 are two cylinders stack upon each other
TEST_F(PatchWeldingTest, WeldStackedCylinders)
{
    loadMap("weld_patches.mapx");

    // Welding the two cylinders produce a 5rows x 9cols patch
    verifyPatchDimensions(performPatchWelding("11", "12"), 5, 9);
}

TEST_F(PatchWeldingTest, WeldedPatchInheritsLayers)
{
    loadMap("weld_patches.mapx");

    auto firstPatch = findPatchWithNumber("1");
    auto firstPatchLayers = firstPatch->getLayers();

    // Check the setup
    EXPECT_EQ(firstPatchLayers, scene::LayerList({ 0, 1 })) << "Patch 1 doesn't have the layers it's expected to, test map changed?";

    // Patch 1 has layers (Default,2), Patch 2 has Layer 2 only
    // After merging we expect the merged patch to have the layers as patch 1 had
    auto mergedNode = performPatchWelding("1", "2");

    EXPECT_EQ(mergedNode->getLayers(), firstPatchLayers);
}

TEST_F(PatchWeldingTest, WeldedPatchInheritsSelectionGroups)
{
    loadMap("weld_patches.mapx");

    auto firstPatch = findPatchWithNumber("1");

    auto firstPatchSelectionGroup = std::dynamic_pointer_cast<IGroupSelectable>(firstPatch);
    auto firstPatchGroups = firstPatchSelectionGroup->getGroupIds();

    // Check the setup
    //EXPECT_EQ(firstPatch, scene::LayerList({ 0, 1 })) << "Patch 1 doesn't have the layers it's expected to, test map changed?";

    // After merging we expect the merged patch to have the same groups as patch 1 had
    auto mergedNode = performPatchWelding("1", "2");
    auto mergedNodeSelectionGroup = std::dynamic_pointer_cast<IGroupSelectable>(firstPatch);
    
    EXPECT_EQ(mergedNodeSelectionGroup->getGroupIds(), firstPatchGroups);
}

}
