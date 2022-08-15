#include "RadiantTest.h"

#include "icommandsystem.h"
#include "ispeakernode.h"
#include "ilightnode.h"
#include "ibrush.h"
#include "scene/PrefabBoundsAccumulator.h"
#include "os/path.h"
#include "algorithm/Scene.h"

namespace test
{

class PrefabTest : public RadiantTest
{
protected:
    void loadPrefabAtOrigin(const std::string& prefabFolderRelativePath)
    {
        fs::path prefabPath = _context.getTestProjectPath();
        prefabPath /= "prefabs";
        prefabPath /= prefabFolderRelativePath;

        GlobalCommandSystem().executeCommand("LoadPrefabAt",
                                             {prefabPath.string(), Vector3(0, 0, 0), 1});
    }
};

TEST_F(PrefabTest, LoadPrefabAt)
{
    loadPrefabAtOrigin("large_bounds.pfbx");
    
    auto numSpeakers = algorithm::getChildCount(GlobalMapModule().getRoot(), [](const scene::INodePtr& node)
    {
        return Node_getSpeakerNode(node) != nullptr;
    });

    auto numLights = algorithm::getChildCount(GlobalMapModule().getRoot(), [](const scene::INodePtr& node)
    {
        return Node_getLightNode(node) != nullptr;
    });

    auto numBrushes = algorithm::getChildCount(GlobalMapModule().getWorldspawn(), [](const scene::INodePtr& node)
    {
        return Node_getIBrush(node) != nullptr;
    });

    EXPECT_EQ(numSpeakers, 1);
    EXPECT_EQ(numLights, 1);
    EXPECT_EQ(numBrushes, 1);
}

TEST_F(PrefabTest, PrefabBoundsCalculation)
{
    loadPrefabAtOrigin("large_bounds.pfbx");

    EXPECT_NE(GlobalSelectionSystem().countSelected(), 0);

    scene::PrefabBoundsAccumulator accumulator;
    GlobalSelectionSystem().foreachSelected(accumulator);

    // Speaker and light bounds should not influence the prefab bounds
    EXPECT_EQ(accumulator.getBounds().getExtents(), Vector3(32, 16, 64));
}

TEST_F(PrefabTest, PrefabBoundsAccumulatorGetNodeBounds)
{
    loadPrefabAtOrigin("large_bounds.pfbx");

    // Prefab bounds calculation should ignore the speaker radius
    auto speaker = algorithm::getEntityByName(GlobalMapModule().getRoot(), "speaker_1");
    auto bounds = scene::PrefabBoundsAccumulator::GetNodeBounds(speaker);
    EXPECT_EQ(bounds.getExtents(), Vector3(8, 8, 8));

    // Prefab bounds calculation should ignore the light volume
    auto light = algorithm::getEntityByName(GlobalMapModule().getRoot(), "light_1");
    bounds = scene::PrefabBoundsAccumulator::GetNodeBounds(light);
    EXPECT_EQ(bounds.getExtents(), Vector3(8, 8, 8));
}

TEST_F(PrefabTest, PrefabInsertPosition)
{
    // The saved prefab is slightly off-center at 128 0 0
    // When inserting it at 0,0,0 the insertion code should compensate the offset

    fs::path prefabPath = _context.getTestProjectPath();
    prefabPath /= "prefabs/large_bounds.pfbx";

    GlobalCommandSystem().executeCommand("LoadPrefabAt",
                                         {prefabPath.string(), Vector3(0, 0, 0), 1});

    auto speaker = algorithm::getEntityByName(GlobalMapModule().getRoot(), "speaker_1");
    EXPECT_EQ(speaker->worldAABB().getOrigin(), Vector3(0, 0, 0));

    auto light = algorithm::getEntityByName(GlobalMapModule().getRoot(), "light_1");
    EXPECT_EQ(light->worldAABB().getOrigin(), Vector3(0, 0, 0));

    auto brush = algorithm::findFirstBrushWithMaterial(GlobalMapModule().getWorldspawn(), "textures/common/caulk");
    EXPECT_EQ(brush->worldAABB().getOrigin(), Vector3(0, 0, 0));
}

TEST_F(PrefabTest, PrefabInsertPositionWithoutOriginCorrection)
{
    // The saved prefab is slightly off-center at 128 0 0
    // When inserting it at 0,0,0 the insertion code should NOT compensate the offset since the
    // fourth argument of LoadPrefabAt is set to recalculatePrefabOrigin == false

    fs::path prefabPath = _context.getTestProjectPath();
    prefabPath /= "prefabs/large_bounds.pfbx";

    GlobalCommandSystem().executeCommand("LoadPrefabAt", cmd::ArgumentList{ prefabPath.string(), Vector3(0, 0, 0), 1, 0 });

    auto speaker = algorithm::getEntityByName(GlobalMapModule().getRoot(), "speaker_1");
    EXPECT_EQ(speaker->worldAABB().getOrigin(), Vector3(128, 0, 0));

    auto light = algorithm::getEntityByName(GlobalMapModule().getRoot(), "light_1");
    EXPECT_EQ(light->worldAABB().getOrigin(), Vector3(128, 0, 0));

    auto brush = algorithm::findFirstBrushWithMaterial(GlobalMapModule().getWorldspawn(), "textures/common/caulk");
    EXPECT_EQ(brush->worldAABB().getOrigin(), Vector3(128, 0, 0));
}

}
