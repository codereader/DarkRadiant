#include "RadiantTest.h"

#include "scene/ShaderBreakdown.h"

namespace test
{

using SceneStatisticsTest = RadiantTest;

TEST_F(SceneStatisticsTest, MaterialBreakDown)
{
    loadMap("material_usage.map");

    scene::ShaderBreakdown breakdown;

    const auto& map = breakdown.getMap();

    // Compare the whole array (Faces, Patches, Models, Particles)
    EXPECT_EQ(map.at("textures/common/collision"), (std::array<std::size_t, 4>({ 0, 0, 1, 0 })));
    EXPECT_EQ(map.at("textures/darkmod/decals/vegetation/moss_patch_thick01"), (std::array<std::size_t, 4>({ 0, 0, 1, 0 })));
    EXPECT_EQ(map.at("textures/numbers/0"), (std::array<std::size_t, 4>({ 6, 2, 0, 0 })));
    EXPECT_EQ(map.at("textures/numbers/1"), (std::array<std::size_t, 4>({ 6, 0, 0, 0 })));
    EXPECT_EQ(map.at("textures/particles/pfiresmall2"), (std::array<std::size_t, 4>({ 0, 0, 0, 1 })));
    EXPECT_EQ(map.at("textures/particles/smokepuff"), (std::array<std::size_t, 4>({ 0, 0, 0, 1 })));
    EXPECT_EQ(map.at("torch"), (std::array<std::size_t, 4>({ 0, 0, 1, 0 })));
    EXPECT_EQ(map.at("torch_shadowcasting"), (std::array<std::size_t, 4>({ 0, 0, 1, 0 })));
}

}
