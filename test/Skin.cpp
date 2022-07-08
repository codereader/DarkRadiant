#include "RadiantTest.h"

#include "modelskin.h"

namespace test
{

using ModelSkinTest = RadiantTest;

TEST_F(ModelSkinTest, FindSkins)
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

    auto& ivyOnesided = GlobalModelSkinCache().capture("ivy_onesided");
    EXPECT_EQ(ivyOnesided.getSkinFileName(), "skins/selection_test.skin");
    EXPECT_EQ(ivyOnesided.getRemap("textures/darkmod/decals/vegetation/ivy_mixed_pieces"), 
        "textures/darkmod/decals/vegetation/ivy_mixed_pieces_onesided");
}

TEST_F(ModelSkinTest, FindSkinsIsCaseInsensitive)
{
    // This is a different spelling than the one used in the decl file
    auto& tileSkin = GlobalModelSkinCache().capture("tILE_skiN");

    EXPECT_NE(tileSkin.getName(), "tILE_skiN") << "Name should not actually be the same as the one in the request";
    EXPECT_EQ(tileSkin.getSkinFileName(), "skins/test_skins.skin");
    EXPECT_EQ(tileSkin.getRemap("textures/atest/a"), "textures/numbers/10");
}

TEST_F(ModelSkinTest, GetRemap)
{
    auto& tileSkin = GlobalModelSkinCache().capture("tile_skin2");

    EXPECT_EQ(tileSkin.getRemap("textures/atest/a"), "textures/numbers/12");
    EXPECT_EQ(tileSkin.getRemap("any_other_texture"), "") << "Missing remap should return an empty string";
}

inline void expectSkinIsListed(const StringList& skins, const std::string& expectedSkin)
{
    EXPECT_NE(std::find(skins.begin(), skins.end(), expectedSkin), skins.end())
        << "Couldn't find the expected skin " << expectedSkin << " in the list";
}

TEST_F(ModelSkinTest, FindMatchingSkins)
{
    auto separatedSkins = GlobalModelSkinCache().getSkinsForModel("models/ase/separated_tiles.ase");
    EXPECT_EQ(separatedSkins.size(), 1);
    EXPECT_EQ(separatedSkins.at(0), "separated_tile_skin");

    auto tileSkins = GlobalModelSkinCache().getSkinsForModel("models/ase/tiles.ase");
    EXPECT_EQ(separatedSkins.size(), 2);
    expectSkinIsListed(tileSkins, "tile_skin");
    expectSkinIsListed(tileSkins, "tile_skin2");
}

TEST_F(ModelSkinTest, GetAllSkins)
{
    auto allSkins = GlobalModelSkinCache().getAllSkins();
    expectSkinIsListed(allSkins, "tile_skin");
    expectSkinIsListed(allSkins, "tile_skin2");
    expectSkinIsListed(allSkins, "separated_tile_skin");
    expectSkinIsListed(allSkins, "skin_with_strange_casing");
    expectSkinIsListed(allSkins, "ivy_onesided");
}



}
