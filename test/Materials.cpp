#include "RadiantTest.h"

#include "ishaders.h"

namespace test
{

using MaterialsTest = RadiantTest;

TEST_F(MaterialsTest, MaterialFileInfo)
{
    auto& materialManager = GlobalMaterialManager();

    // Expect our example material definitions in the ShaderLibrary
    EXPECT_TRUE(materialManager.materialExists("textures/orbweaver/drain_grille"));
    EXPECT_TRUE(materialManager.materialExists("models/md5/chars/nobles/noblewoman/noblebottom"));
    EXPECT_TRUE(materialManager.materialExists("tdm_spider_black"));

    // ShaderDefinitions should contain their source file infos
    const auto& drainGrille = materialManager.getMaterialForName("textures/orbweaver/drain_grille");
    EXPECT_EQ(drainGrille->getShaderFileInfo()->name, "example.mtr");
    EXPECT_EQ(drainGrille->getShaderFileInfo()->visibility, vfs::Visibility::NORMAL);

    const auto& nobleTop = materialManager.getMaterialForName("models/md5/chars/nobles/noblewoman/nobletop");
    EXPECT_EQ(nobleTop->getShaderFileInfo()->name, "tdm_ai_nobles.mtr");
    EXPECT_EQ(nobleTop->getShaderFileInfo()->visibility, vfs::Visibility::NORMAL);

    // Visibility should be parsed from assets.lst
    const auto& hiddenTex = materialManager.getMaterialForName("textures/orbweaver/drain_grille_h");
    EXPECT_EQ(hiddenTex->getShaderFileInfo()->name, "hidden.mtr");
    EXPECT_EQ(hiddenTex->getShaderFileInfo()->visibility, vfs::Visibility::HIDDEN);

    // assets.lst visibility applies to the MTR file, and should propagate to
    // all shaders within it
    const auto& hiddenTex2 = materialManager.getMaterialForName("textures/darkmod/another_white");
    EXPECT_EQ(hiddenTex2->getShaderFileInfo()->name, "hidden.mtr");
    EXPECT_EQ(hiddenTex2->getShaderFileInfo()->visibility, vfs::Visibility::HIDDEN);
}

TEST_F(MaterialsTest, MaterialParser)
{
    auto& materialManager = GlobalMaterialManager();

    // All of these materials need to be present
    // variant3 lacks whitespace between its name and {, which caused trouble in #4900
    EXPECT_TRUE(materialManager.materialExists("textures/parsing_test/variant1"));
    EXPECT_TRUE(materialManager.materialExists("textures/parsing_test/variant2"));
    EXPECT_TRUE(materialManager.materialExists("textures/parsing_test/variant3"));
}

}
