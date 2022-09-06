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

inline void compareNormalOfFirstSharedVertex(const PatchMesh& mesh1, const PatchMesh& mesh2)
{
    // Find the first matching 3D vertex and return it
    for (const auto& v1 : mesh1.vertices)
    {
        for (const auto& v2 : mesh2.vertices)
        {
            if (math::isNear(v1.vertex, v2.vertex, 0.01))
            {
                EXPECT_LT(std::abs(v1.normal.angle(v2.normal)), c_half_pi);
                return;
            }
        }
    }

    EXPECT_FALSE(true) << "Didn't find any matching mesh vertex";
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

    // Check patch orientation
    auto& refPatch = std::dynamic_pointer_cast<IPatchNode>(firstPatch)->getPatch();

    auto mergedPatchNode = GlobalSelectionSystem().ultimateSelected();
    auto& mergedPatch = std::dynamic_pointer_cast<IPatchNode>(mergedPatchNode)->getPatch();

    compareNormalOfFirstSharedVertex(refPatch.getTesselatedPatchMesh(), mergedPatch.getTesselatedPatchMesh());

    return GlobalSelectionSystem().ultimateSelected();
}

void verifyMergedPatch(const scene::INodePtr& mergedPatchNode, int expectedRows, int expectedCols)
{
    auto merged = std::dynamic_pointer_cast<IPatchNode>(mergedPatchNode);
    EXPECT_EQ(merged->getPatch().getHeight(), expectedRows);
    EXPECT_EQ(merged->getPatch().getWidth(), expectedCols);
}

void assumePatchWeldingFails(const scene::INodePtr& firstPatch, const scene::INodePtr& secondPatch)
{
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

void assumePatchWeldingFails(const std::string& number1, const std::string& number2)
{
    auto firstPatch = findPatchWithNumber(number1);
    auto secondPatch = findPatchWithNumber(number2);

    assumePatchWeldingFails(firstPatch, secondPatch);
}

}

TEST_P(PatchWelding3x3, WeldWithOther3x3Patch)
{
    loadMap("weld_patches2.mapx");

    auto firstPatch = std::get<0>(GetParam());
    auto secondPatch = std::get<1>(GetParam());
    auto expectedRows = std::get<2>(GetParam());
    auto expectedColumns = std::get<3>(GetParam());

    verifyMergedPatch(performPatchWelding(firstPatch, secondPatch), expectedRows, expectedColumns);
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

TEST_F(PatchWeldingTest, WeldPatchExceedingMaximumDimensions)
{
    loadMap("weld_patches_out_of_bounds.map");

    auto firstPatch = algorithm::getNthChild(GlobalMapModule().findOrInsertWorldspawn(), 0);
    auto secondPatch = algorithm::getNthChild(GlobalMapModule().findOrInsertWorldspawn(), 1);

    EXPECT_TRUE(Node_isPatch(firstPatch)) << "Map should contain excactly two worldspawn patches";
    EXPECT_TRUE(Node_isPatch(secondPatch)) << "Map should contain excactly two worldspawn patches";

    assumePatchWeldingFails(firstPatch, secondPatch);
}

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
    verifyMergedPatch(performPatchWelding("11", "12"), 5, 9);
}

TEST_F(PatchWeldingTest, WeldStackedCylindersOtherWayAround)
{
    loadMap("weld_patches.mapx");

    // Welding the two cylinders produce a 5rows x 9cols patch
    verifyMergedPatch(performPatchWelding("12", "11"), 5, 9);
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

TEST_F(PatchWeldingTest, WeldingPreservesFixedSubdivisions)
{
    loadMap("weld_patches.mapx");

    // Patch 1 has fixed subdivisions 5 horizontal, 2 vertical
    auto firstPatch = findPatchWithNumber("1");
    auto firstPatchNode = std::dynamic_pointer_cast<IPatchNode>(firstPatch);

    // Check the setup
    EXPECT_TRUE(firstPatchNode->getPatch().subdivisionsFixed()) << "Patch 1 isn't set to fixed subdivisions 5x2, test map changed?";
    EXPECT_EQ(firstPatchNode->getPatch().getSubdivisions(), Subdivisions(5, 2)) << "Patch 1 isn't set to fixed subdivisions 5x2, test map changed?";

    // After merging we expect the merged patch to have the same subdivisions as patch 1 had
    auto mergedNode = performPatchWelding("1", "2");

    auto merged = std::dynamic_pointer_cast<IPatchNode>(mergedNode);

    EXPECT_TRUE(merged->getPatch().subdivisionsFixed()) << "Merged patch is supposed to have fixed subdivisions 5x2";
    EXPECT_EQ(merged->getPatch().getSubdivisions(), Subdivisions(5, 2)) << "Merged patch is supposed to have fixed subdivisions 5x2";
}

TEST_F(PatchWeldingTest, WeldingPreservesNonfixedSubdivisions)
{
    loadMap("weld_patches.mapx");

    // Patch 1 has fixed subdivisions 5 horizontal, 2 vertical, patch 2 has nothing set
    auto firstPatch = findPatchWithNumber("2");
    auto firstPatchNode = std::dynamic_pointer_cast<IPatchNode>(firstPatch);

    // Check the setup
    EXPECT_FALSE(firstPatchNode->getPatch().subdivisionsFixed()) << "Patch 2 is set to fixed subdivisions, test map changed?";

    // After merging we expect the merged patch to have the same subdivisions as patch 2 had
    auto mergedNode = performPatchWelding("2", "1");

    auto merged = std::dynamic_pointer_cast<IPatchNode>(mergedNode);

    EXPECT_FALSE(merged->getPatch().subdivisionsFixed()) << "Merged patch should not have fixed subdivisions set";
}

TEST_F(PatchWeldingTest, WeldingInheritsMaterial)
{
    loadMap("weld_patches.mapx");

    // Patch 1 has material "1"
    auto firstPatch = findPatchWithNumber("1");
    auto firstPatchNode = std::dynamic_pointer_cast<IPatchNode>(firstPatch);

    // Check the setup
    EXPECT_EQ(firstPatchNode->getPatch().getShader(), "textures/numbers/1") << "Patch 1 should have textures/numbers/1, test map changed?";

    // After merging we expect the merged patch to have the same material as patch 1 had
    auto mergedNode = performPatchWelding("1", "2");

    auto merged = std::dynamic_pointer_cast<IPatchNode>(mergedNode);

    EXPECT_EQ(merged->getPatch().getShader(), "textures/numbers/1") << "Merged patch should have textures/numbers/1";
}

TEST_F(PatchWeldingTest, WeldMultipleSelectedPatches)
{
    loadMap("weld_patches.mapx");

    // Select all patches with the material 13
    GlobalMapModule().findOrInsertWorldspawn()->foreachNode([&](const scene::INodePtr& node)
    {
        auto patchNode = std::dynamic_pointer_cast<IPatchNode>(node);

        if (patchNode && patchNode->getPatch().getShader() == "textures/numbers/13")
        {
            Node_setSelected(node, true);
        }

        return true;
    });

    // Check the setup
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 6) << "Expected 6 patches with material 13, test map changed?";

    // Execute the weld patches command three times, regardless of the order only one should remain
    GlobalCommandSystem().executeCommand("WeldSelectedPatches");
    GlobalCommandSystem().executeCommand("WeldSelectedPatches");
    GlobalCommandSystem().executeCommand("WeldSelectedPatches");

    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "Expected 1 remaining patch with material 13";

    // We might get a 7x9 or a 9x7 patch
    auto& result = std::dynamic_pointer_cast<IPatchNode>(GlobalSelectionSystem().ultimateSelected())->getPatch();

    EXPECT_EQ(std::max(result.getWidth(), result.getHeight()), 9);
    EXPECT_EQ(std::min(result.getWidth(), result.getHeight()), 7);
}


TEST_F(PatchWeldingTest, WeldingIsUndoable)
{
    loadMap("weld_patches.mapx");

    {
        auto firstPatch = findPatchWithNumber("1");
        auto secondPatch = findPatchWithNumber("2");

        EXPECT_TRUE(firstPatch && firstPatch->getParent());
        EXPECT_TRUE(secondPatch && secondPatch->getParent());

        Node_setSelected(firstPatch, true);
        Node_setSelected(secondPatch, true);
    }

    GlobalCommandSystem().executeCommand("WeldSelectedPatches");

    // Patch 2 should be gone now
    EXPECT_TRUE(findPatchWithNumber("1")); // this is the merged patch
    EXPECT_FALSE(findPatchWithNumber("2"));

    GlobalCommandSystem().executeCommand("Undo");

    // Both patches should be back again
    EXPECT_TRUE(findPatchWithNumber("1"));
    EXPECT_TRUE(findPatchWithNumber("2"));
}

}
