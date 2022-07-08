#include "RadiantTest.h"

#include "modelskin.h"

namespace test
{

using ModelSkinTest = RadiantTest;

TEST_F(ModelSkinTest, FindSkinDecl)
{
    // All of these declarations need to be parsed and present
    auto& tileSkin = GlobalModelSkinCache().capture("tile_skin");
    EXPECT_EQ(tileSkin.getSkinFileName(), "skins/test_skins.skin");
    EXPECT_EQ(tileSkin.getRemap("textures/atest/a"), "textures/numbers/10");

    auto& separatedTileSkin = GlobalModelSkinCache().capture("separated_tile_skin");
    EXPECT_EQ(separatedTileSkin.getSkinFileName(), "skins/test_skins.skin");
    EXPECT_EQ(separatedTileSkin.getRemap("material"), "textures/numbers/11");

    auto& skinWithStrangeCasing = GlobalModelSkinCache().capture("skin_with_strange_casing");
    EXPECT_EQ(skinWithStrangeCasing.getSkinFileName(), "skins/test_skins.skin");
    EXPECT_EQ(skinWithStrangeCasing.getRemap("material"), "textures/numbers/11");
}

}
