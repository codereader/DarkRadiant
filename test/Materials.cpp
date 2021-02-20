#include "RadiantTest.h"

#include "ishaders.h"
#include <algorithm>
#include "string/split.h"
#include "string/case_conv.h"
#include "string/trim.h"

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
    EXPECT_EQ(drainGrille->getShaderFileInfo().name, "example.mtr");
    EXPECT_EQ(drainGrille->getShaderFileInfo().visibility, vfs::Visibility::NORMAL);

    const auto& nobleTop = materialManager.getMaterialForName("models/md5/chars/nobles/noblewoman/nobletop");
    EXPECT_EQ(nobleTop->getShaderFileInfo().name, "tdm_ai_nobles.mtr");
    EXPECT_EQ(nobleTop->getShaderFileInfo().visibility, vfs::Visibility::NORMAL);

    // Visibility should be parsed from assets.lst
    const auto& hiddenTex = materialManager.getMaterialForName("textures/orbweaver/drain_grille_h");
    EXPECT_EQ(hiddenTex->getShaderFileInfo().name, "hidden.mtr");
    EXPECT_EQ(hiddenTex->getShaderFileInfo().visibility, vfs::Visibility::HIDDEN);

    // assets.lst visibility applies to the MTR file, and should propagate to
    // all shaders within it
    const auto& hiddenTex2 = materialManager.getMaterialForName("textures/darkmod/another_white");
    EXPECT_EQ(hiddenTex2->getShaderFileInfo().name, "hidden.mtr");
    EXPECT_EQ(hiddenTex2->getShaderFileInfo().visibility, vfs::Visibility::HIDDEN);
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

TEST_F(MaterialsTest, MaterialParserPolygonOffset)
{
    auto& materialManager = GlobalMaterialManager();

    auto polygonOffset1 = materialManager.getMaterialForName("textures/parsertest/polygonOffset1");

    EXPECT_TRUE(polygonOffset1->getMaterialFlags() & Material::FLAG_POLYGONOFFSET);
    EXPECT_EQ(polygonOffset1->getPolygonOffset(), 1.0f) << "Default value of polygonOffset should be 1.0";

    auto polygonOffset2 = materialManager.getMaterialForName("textures/parsertest/polygonOffset2");

    EXPECT_TRUE(polygonOffset2->getMaterialFlags() & Material::FLAG_POLYGONOFFSET);
    EXPECT_EQ(polygonOffset2->getPolygonOffset(), 13.0f);

    auto polygonOffset3 = materialManager.getMaterialForName("textures/parsertest/polygonOffset3");

    EXPECT_TRUE(polygonOffset3->getMaterialFlags() & Material::FLAG_POLYGONOFFSET);
    EXPECT_EQ(polygonOffset3->getPolygonOffset(), -3.0f);
}

TEST_F(MaterialsTest, MaterialParserSortRequest)
{
    auto& materialManager = GlobalMaterialManager();

    auto material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_none");
    EXPECT_FALSE(material->getParseFlags() & Material::PF_HasSortDefined);

    material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_subview");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_SUBVIEW);

    material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_opaque");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_OPAQUE);

    material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_decal");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_DECAL);

    material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_far");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_FAR);

    material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_medium");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_MEDIUM);

    material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_close");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_CLOSE);

    material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_almostnearest");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_ALMOST_NEAREST);

    material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_nearest");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_NEAREST);

    material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_afterfog");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_AFTER_FOG);

    material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_postprocess");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_POST_PROCESS);

    material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_portalsky");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_PORTAL_SKY);
    
    material = materialManager.getMaterialForName("textures/parsertest/sortPredefined_decal_macro");
    EXPECT_FALSE(material->getParseFlags() & Material::PF_HasSortDefined); // sort is not explicitly set
    EXPECT_EQ(material->getSortRequest(), Material::SORT_DECAL);
}

TEST_F(MaterialsTest, MaterialParserAmbientRimColour)
{
    auto material = GlobalMaterialManager().getMaterialForName("textures/parsertest/withAmbientRimColor");

    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasAmbientRimColour);
    // no further support for ambientRimColor at this point
}

TEST_F(MaterialsTest, MaterialParserSpectrum)
{
    auto material = GlobalMaterialManager().getMaterialForName("textures/parsertest/nospectrum");

    EXPECT_FALSE(material->getParseFlags() & Material::PF_HasSpectrum);
    EXPECT_EQ(material->getSpectrum(), 0);

    material = GlobalMaterialManager().getMaterialForName("textures/parsertest/spectrumMinus45");

    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSpectrum);
    EXPECT_EQ(material->getSpectrum(), -45);

    material = GlobalMaterialManager().getMaterialForName("textures/parsertest/spectrumPositive100");

    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSpectrum);
    EXPECT_EQ(material->getSpectrum(), 100);
}


inline void checkRenderBumpArguments(const std::string& materialName)
{
    auto material = GlobalMaterialManager().getMaterialForName(materialName);

    auto fullDefinition = material->getDefinition();
    std::vector<std::string> lines;
    string::split(lines, fullDefinition, "\n", true);

    auto renderBumpLine = std::find_if(lines.begin(), lines.end(),
        [](const std::string& line) { return line.find("renderBump") != std::string::npos; });

    EXPECT_NE(renderBumpLine, lines.end());
    string::to_lower(*renderBumpLine);

    std::string renderBumpKeyword = "renderbump";
    auto args = renderBumpLine->substr(renderBumpLine->find(renderBumpKeyword) + renderBumpKeyword.size());

    string::trim(args);

    EXPECT_EQ(material->getRenderBumpArguments(), args);
}
TEST_F(MaterialsTest, MaterialParserRenderbump)
{
    checkRenderBumpArguments("textures/parsertest/renderBump1");
    checkRenderBumpArguments("textures/parsertest/renderBump2");
    checkRenderBumpArguments("textures/parsertest/renderBump3");
    checkRenderBumpArguments("textures/parsertest/renderBump4");
    checkRenderBumpArguments("textures/parsertest/renderBump5");
    checkRenderBumpArguments("textures/parsertest/renderBump6");
}

}
