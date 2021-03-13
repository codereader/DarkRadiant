#include "RadiantTest.h"

#include "ishaders.h"
#include <algorithm>
#include "string/split.h"
#include "string/case_conv.h"
#include "string/trim.h"
#include "string/join.h"

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
    const auto& drainGrille = materialManager.getMaterial("textures/orbweaver/drain_grille");
    EXPECT_EQ(drainGrille->getShaderFileInfo().name, "example.mtr");
    EXPECT_EQ(drainGrille->getShaderFileInfo().visibility, vfs::Visibility::NORMAL);

    const auto& nobleTop = materialManager.getMaterial("models/md5/chars/nobles/noblewoman/nobletop");
    EXPECT_EQ(nobleTop->getShaderFileInfo().name, "tdm_ai_nobles.mtr");
    EXPECT_EQ(nobleTop->getShaderFileInfo().visibility, vfs::Visibility::NORMAL);

    // Visibility should be parsed from assets.lst
    const auto& hiddenTex = materialManager.getMaterial("textures/orbweaver/drain_grille_h");
    EXPECT_EQ(hiddenTex->getShaderFileInfo().name, "hidden.mtr");
    EXPECT_EQ(hiddenTex->getShaderFileInfo().visibility, vfs::Visibility::HIDDEN);

    // assets.lst visibility applies to the MTR file, and should propagate to
    // all shaders within it
    const auto& hiddenTex2 = materialManager.getMaterial("textures/darkmod/another_white");
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

TEST_F(MaterialsTest, EnumerateMaterialLayers)
{
    auto material = GlobalMaterialManager().getMaterial("tdm_spider_black");
    EXPECT_TRUE(material);

    // Get a list of all layers in the material
    auto layers = material->getAllLayers();
    EXPECT_EQ(layers.size(), 5);

    // First layer is the bump map in this particular material
    EXPECT_EQ(layers.at(0)->getType(), IShaderLayer::BUMP);
    EXPECT_EQ(layers.at(0)->getMapImageFilename(),
              "models/md5/chars/monsters/spider/spider_local");

    // Second layer is the diffuse map
    EXPECT_EQ(layers.at(1)->getType(), IShaderLayer::DIFFUSE);
    EXPECT_EQ(layers.at(1)->getMapImageFilename(),
              "models/md5/chars/monsters/spider_black");

    // Third layer is the specular map
    EXPECT_EQ(layers.at(2)->getType(), IShaderLayer::SPECULAR);
    EXPECT_EQ(layers.at(2)->getMapImageFilename(),
              "models/md5/chars/monsters/spider_s");

    // Fourth layer is the additive "ambient method" stage
    EXPECT_EQ(layers.at(3)->getType(), IShaderLayer::BLEND);
    EXPECT_EQ(layers.at(3)->getMapImageFilename(),
              "models/md5/chars/monsters/spider_black");
    BlendFunc bf4 = layers.at(3)->getBlendFunc();
    EXPECT_EQ(bf4.src, GL_ONE);
    EXPECT_EQ(bf4.dest, GL_ONE);

    // Fifth layer is another additive stage with a VFP
    EXPECT_EQ(layers.at(4)->getType(), IShaderLayer::BLEND);
    EXPECT_EQ(layers.at(4)->getNumFragmentMaps(), 4);
    BlendFunc bf5 = layers.at(4)->getBlendFunc();
    EXPECT_EQ(bf5.src, GL_ONE);
    EXPECT_EQ(bf5.dest, GL_ONE);
}

TEST_F(MaterialsTest, IdentifyAmbientLight)
{
    auto ambLight = GlobalMaterialManager().getMaterial("lights/ambientLight");
    ASSERT_TRUE(ambLight);
    EXPECT_TRUE(ambLight->isAmbientLight());

    auto pointLight = GlobalMaterialManager().getMaterial("lights/defaultPointLight");
    ASSERT_TRUE(pointLight);
    EXPECT_FALSE(pointLight->isAmbientLight());

    auto nonLight = GlobalMaterialManager().getMaterial("tdm_spider_black");
    ASSERT_TRUE(nonLight);
    EXPECT_FALSE(nonLight->isAmbientLight());
}

TEST_F(MaterialsTest, MaterialParserPolygonOffset)
{
    auto& materialManager = GlobalMaterialManager();

    auto polygonOffset1 = materialManager.getMaterial("textures/parsertest/polygonOffset1");

    EXPECT_TRUE(polygonOffset1->getMaterialFlags() & Material::FLAG_POLYGONOFFSET);
    EXPECT_EQ(polygonOffset1->getPolygonOffset(), 1.0f) << "Default value of polygonOffset should be 1.0";

    auto polygonOffset2 = materialManager.getMaterial("textures/parsertest/polygonOffset2");

    EXPECT_TRUE(polygonOffset2->getMaterialFlags() & Material::FLAG_POLYGONOFFSET);
    EXPECT_EQ(polygonOffset2->getPolygonOffset(), 13.0f);

    auto polygonOffset3 = materialManager.getMaterial("textures/parsertest/polygonOffset3");

    EXPECT_TRUE(polygonOffset3->getMaterialFlags() & Material::FLAG_POLYGONOFFSET);
    EXPECT_EQ(polygonOffset3->getPolygonOffset(), -3.0f);
}
TEST_F(MaterialsTest, MaterialParserSortRequest)
{
    auto& materialManager = GlobalMaterialManager();

    auto material = materialManager.getMaterial("textures/parsertest/sortPredefined_none");
    EXPECT_FALSE(material->getParseFlags() & Material::PF_HasSortDefined);

    material = materialManager.getMaterial("textures/parsertest/sortPredefined_subview");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_SUBVIEW);

    material = materialManager.getMaterial("textures/parsertest/sortPredefined_opaque");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_OPAQUE);

    material = materialManager.getMaterial("textures/parsertest/sortPredefined_decal");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_DECAL);

    material = materialManager.getMaterial("textures/parsertest/sortPredefined_far");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_FAR);

    material = materialManager.getMaterial("textures/parsertest/sortPredefined_medium");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_MEDIUM);

    material = materialManager.getMaterial("textures/parsertest/sortPredefined_close");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_CLOSE);

    material = materialManager.getMaterial("textures/parsertest/sortPredefined_almostnearest");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_ALMOST_NEAREST);

    material = materialManager.getMaterial("textures/parsertest/sortPredefined_nearest");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_NEAREST);

    material = materialManager.getMaterial("textures/parsertest/sortPredefined_afterfog");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_AFTER_FOG);

    material = materialManager.getMaterial("textures/parsertest/sortPredefined_postprocess");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_POST_PROCESS);

    material = materialManager.getMaterial("textures/parsertest/sortPredefined_portalsky");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_EQ(material->getSortRequest(), Material::SORT_PORTAL_SKY);
    
    material = materialManager.getMaterial("textures/parsertest/sortPredefined_decal_macro");
    EXPECT_FALSE(material->getParseFlags() & Material::PF_HasSortDefined); // sort is not explicitly set
    EXPECT_EQ(material->getSortRequest(), Material::SORT_DECAL);
}

TEST_F(MaterialsTest, MaterialParserAmbientRimColour)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/withAmbientRimColor");

    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasAmbientRimColour);
    // no further support for ambientRimColor at this point
}

TEST_F(MaterialsTest, MaterialParserSpectrum)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/nospectrum");

    EXPECT_FALSE(material->getParseFlags() & Material::PF_HasSpectrum);
    EXPECT_EQ(material->getSpectrum(), 0);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/spectrumMinus45");

    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSpectrum);
    EXPECT_EQ(material->getSpectrum(), -45);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/spectrumPositive100");

    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSpectrum);
    EXPECT_EQ(material->getSpectrum(), 100);
}

inline void checkRenderBumpArguments(const std::string& materialName, const std::string& keyword)
{
    auto material = GlobalMaterialManager().getMaterial(materialName);

    auto fullDefinition = material->getDefinition();
    std::vector<std::string> lines;
    string::split(lines, fullDefinition, "\n", true);

    std::string keywordLower = string::to_lower_copy(keyword);
    auto renderBumpLine = std::find_if(lines.begin(), lines.end(),
        [&](const std::string& line) { return string::to_lower_copy(line).find(keywordLower) != std::string::npos; });

    EXPECT_NE(renderBumpLine, lines.end());

    std::string line = *renderBumpLine;
    auto args = line.substr(string::to_lower_copy(line).find(keywordLower) + keywordLower.size());

    string::trim(args);

    if (keyword == "renderbump")
    {
        EXPECT_EQ(material->getRenderBumpArguments(), args);
    }
    else
    {
        EXPECT_EQ(material->getRenderBumpFlatArguments(), args);
    }
}

TEST_F(MaterialsTest, MaterialParserRenderbump)
{
    checkRenderBumpArguments("textures/parsertest/renderBump1", "renderbump");
    checkRenderBumpArguments("textures/parsertest/renderBump2", "renderbump");
    checkRenderBumpArguments("textures/parsertest/renderBump3", "renderbump");
    checkRenderBumpArguments("textures/parsertest/renderBump4", "renderbump");
    checkRenderBumpArguments("textures/parsertest/renderBump5", "renderbump");
    checkRenderBumpArguments("textures/parsertest/renderBump6", "renderbump");
}

TEST_F(MaterialsTest, MaterialParserRenderbumpFlat)
{
    checkRenderBumpArguments("textures/parsertest/renderBumpFlat1", "renderbumpflat");
    checkRenderBumpArguments("textures/parsertest/renderBumpFlat2", "renderbumpflat");
}

TEST_F(MaterialsTest, MaterialParserDeform)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/deform1");
    EXPECT_EQ(material->getDeformType(), Material::DEFORM_FLARE);
    EXPECT_EQ(material->getDeformDeclName(), "");
    EXPECT_EQ(material->getDeformExpression(0)->getExpressionString(), "1.5");
    EXPECT_FALSE(material->getDeformExpression(1));
    EXPECT_FALSE(material->getDeformExpression(2));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/deform2");
    EXPECT_EQ(material->getDeformType(), Material::DEFORM_EXPAND);
    EXPECT_EQ(material->getDeformDeclName(), "");
    EXPECT_EQ(material->getDeformExpression(0)->getExpressionString(), "(0.1 * deformtesttable[time * (0.3 + time)] - global3)");
    EXPECT_FALSE(material->getDeformExpression(1));
    EXPECT_FALSE(material->getDeformExpression(2));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/deform3");
    EXPECT_EQ(material->getDeformType(), Material::DEFORM_MOVE);
    EXPECT_EQ(material->getDeformDeclName(), "");
    EXPECT_EQ(material->getDeformExpression(0)->getExpressionString(), "(1.7 + time + 4.0 - global3)");
    EXPECT_FALSE(material->getDeformExpression(1));
    EXPECT_FALSE(material->getDeformExpression(2));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/deform4");
    EXPECT_EQ(material->getDeformType(), Material::DEFORM_TURBULENT);
    EXPECT_EQ(material->getDeformDeclName(), "deformtesttable");
    EXPECT_EQ(material->getDeformExpression(0)->getExpressionString(), "time * 2.0");
    EXPECT_EQ(material->getDeformExpression(1)->getExpressionString(), "(parm11 - 4.0)");
    EXPECT_EQ(material->getDeformExpression(2)->getExpressionString(), "-1.0 * global5");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/deform5");
    EXPECT_EQ(material->getDeformType(), Material::DEFORM_PARTICLE);
    EXPECT_EQ(material->getDeformDeclName(), "testparticle");
    EXPECT_FALSE(material->getDeformExpression(0));
    EXPECT_FALSE(material->getDeformExpression(1));
    EXPECT_FALSE(material->getDeformExpression(2));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/deform6");
    EXPECT_EQ(material->getDeformType(), Material::DEFORM_PARTICLE2);
    EXPECT_EQ(material->getDeformDeclName(), "testparticle");
    EXPECT_FALSE(material->getDeformExpression(0));
    EXPECT_FALSE(material->getDeformExpression(1));
    EXPECT_FALSE(material->getDeformExpression(2));
}

TEST_F(MaterialsTest, MaterialParserStageTranslate)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/notransform");
    EXPECT_FALSE(material->getAllLayers().front()->getTranslationExpression(0));
    EXPECT_FALSE(material->getAllLayers().front()->getTranslationExpression(1));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/translation1");
    EXPECT_EQ(material->getAllLayers().front()->getTranslationExpression(0)->getExpressionString(), "3.0");
    EXPECT_EQ(material->getAllLayers().front()->getTranslationExpression(1)->getExpressionString(), "parm3 * 3.0");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/translation2");
    EXPECT_EQ(material->getAllLayers().front()->getTranslationExpression(0)->getExpressionString(), "time");
    EXPECT_EQ(material->getAllLayers().front()->getTranslationExpression(1)->getExpressionString(), "0.5");
}

TEST_F(MaterialsTest, MaterialParserStageRotate)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/notransform");
    EXPECT_FALSE(material->getAllLayers().front()->getRotationExpression());

    material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/rotate1");
    EXPECT_EQ(material->getAllLayers().front()->getRotationExpression()->getExpressionString(), "0.03");
}

TEST_F(MaterialsTest, MaterialParserStageScale)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/notransform");
    EXPECT_FALSE(material->getAllLayers().front()->getScaleExpression(0));
    EXPECT_FALSE(material->getAllLayers().front()->getScaleExpression(1));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/scale1");
    EXPECT_EQ(material->getAllLayers().front()->getScaleExpression(0)->getExpressionString(), "4.0");
    EXPECT_EQ(material->getAllLayers().front()->getScaleExpression(1)->getExpressionString(), "time * 3.0");
}

TEST_F(MaterialsTest, MaterialParserStageCenterScale)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/notransform");
    EXPECT_FALSE(material->getAllLayers().front()->getCenterScaleExpression(0));
    EXPECT_FALSE(material->getAllLayers().front()->getCenterScaleExpression(1));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/centerscale1");
    EXPECT_EQ(material->getAllLayers().front()->getCenterScaleExpression(0)->getExpressionString(), "4.0");
    EXPECT_EQ(material->getAllLayers().front()->getCenterScaleExpression(1)->getExpressionString(), "time * 3.0");
}

TEST_F(MaterialsTest, MaterialParserStageShear)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/notransform");
    EXPECT_FALSE(material->getAllLayers().front()->getShearExpression(0));
    EXPECT_FALSE(material->getAllLayers().front()->getShearExpression(1));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/shear1");
    EXPECT_EQ(material->getAllLayers().front()->getShearExpression(0)->getExpressionString(), "global3");
    EXPECT_EQ(material->getAllLayers().front()->getShearExpression(1)->getExpressionString(), "4.0");
}

TEST_F(MaterialsTest, MaterialParserStageVertexProgram)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram1");
    material->getAllLayers().front()->evaluateExpressions(0);

    EXPECT_EQ(material->getAllLayers().front()->getNumVertexParms(), 1);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParmValue(0), Vector4(0, 0, 0, 0)); // all 4 equal
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).index, 0);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[0]->getExpressionString(), "time");
    EXPECT_FALSE(material->getAllLayers().front()->getVertexParm(0).expressions[1]);
    EXPECT_FALSE(material->getAllLayers().front()->getVertexParm(0).expressions[2]);
    EXPECT_FALSE(material->getAllLayers().front()->getVertexParm(0).expressions[3]);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram2");
    material->getAllLayers().front()->evaluateExpressions(0);

    EXPECT_EQ(material->getAllLayers().front()->getNumVertexParms(), 1);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParmValue(0), Vector4(0, 3, 0, 1)); // z=0,w=1 implicitly
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).index, 0);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[0]->getExpressionString(), "time");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[1]->getExpressionString(), "3.0");
    EXPECT_FALSE(material->getAllLayers().front()->getVertexParm(0).expressions[2]);
    EXPECT_FALSE(material->getAllLayers().front()->getVertexParm(0).expressions[3]);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram3");
    material->getAllLayers().front()->evaluateExpressions(0);

    EXPECT_EQ(material->getAllLayers().front()->getNumVertexParms(), 1);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParmValue(0), Vector4(0, 3, 0, 1)); // w=1 implicitly
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).index, 0);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[0]->getExpressionString(), "time");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[1]->getExpressionString(), "3.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[2]->getExpressionString(), "global3");
    EXPECT_FALSE(material->getAllLayers().front()->getVertexParm(0).expressions[3]);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram4");
    material->getAllLayers().front()->evaluateExpressions(1000); // time = 1 sec

    EXPECT_EQ(material->getAllLayers().front()->getNumVertexParms(), 1);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParmValue(0), Vector4(1, 3, 0, 2));
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).index, 0);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[0]->getExpressionString(), "time");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[1]->getExpressionString(), "3.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[2]->getExpressionString(), "global3");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[3]->getExpressionString(), "time * 2.0");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram5");
    material->getAllLayers().front()->evaluateExpressions(2000); // time = 2 secs

    EXPECT_EQ(material->getAllLayers().front()->getNumVertexParms(), 3);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParmValue(0), Vector4(2, 3, 0, 4));

    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).index, 0);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[0]->getExpressionString(), "time");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[1]->getExpressionString(), "3.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[2]->getExpressionString(), "global3");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[3]->getExpressionString(), "time * 2.0");

    EXPECT_EQ(material->getAllLayers().front()->getVertexParmValue(1), Vector4(1, 2, 3, 4));
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(1).index, 1);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(1).expressions[0]->getExpressionString(), "1.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(1).expressions[1]->getExpressionString(), "2.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(1).expressions[2]->getExpressionString(), "3.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(1).expressions[3]->getExpressionString(), "4.0");

    EXPECT_EQ(material->getAllLayers().front()->getVertexParmValue(2), Vector4(5, 6, 7, 8));
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(2).index, 2);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(2).expressions[0]->getExpressionString(), "5.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(2).expressions[1]->getExpressionString(), "6.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(2).expressions[2]->getExpressionString(), "7.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(2).expressions[3]->getExpressionString(), "8.0");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram6");
    material->getAllLayers().front()->evaluateExpressions(2000); // time = 2 secs

    EXPECT_EQ(material->getAllLayers().front()->getNumVertexParms(), 3);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParmValue(0), Vector4(2, 3, 0, 4));

    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).index, 0);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[0]->getExpressionString(), "time");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[1]->getExpressionString(), "3.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[2]->getExpressionString(), "global3");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(0).expressions[3]->getExpressionString(), "time * 2.0");

    EXPECT_EQ(material->getAllLayers().front()->getVertexParmValue(1), Vector4(0, 0, 0, 0));
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(1).index, -1); // missing
    EXPECT_FALSE(material->getAllLayers().front()->getVertexParm(1).expressions[0]);
    EXPECT_FALSE(material->getAllLayers().front()->getVertexParm(1).expressions[1]);
    EXPECT_FALSE(material->getAllLayers().front()->getVertexParm(1).expressions[2]);
    EXPECT_FALSE(material->getAllLayers().front()->getVertexParm(1).expressions[3]);

    EXPECT_EQ(material->getAllLayers().front()->getVertexParmValue(2), Vector4(5, 6, 7, 8));
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(2).index, 2);
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(2).expressions[0]->getExpressionString(), "5.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(2).expressions[1]->getExpressionString(), "6.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(2).expressions[2]->getExpressionString(), "7.0");
    EXPECT_EQ(material->getAllLayers().front()->getVertexParm(2).expressions[3]->getExpressionString(), "8.0");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram7");
    EXPECT_TRUE(material->getAllLayers().empty()); // failure to parse should end up with an empty material
}

TEST_F(MaterialsTest, MaterialParserStageFragmentProgram)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/program/fragmentProgram1");

    EXPECT_EQ(material->getAllLayers().front()->getFragmentProgram(), "glprogs/test.vfp");
    EXPECT_EQ(material->getAllLayers().front()->getNumFragmentMaps(), 3);
    EXPECT_EQ(material->getAllLayers().front()->getFragmentMap(0).index, 0);
    EXPECT_EQ(string::join(material->getAllLayers().front()->getFragmentMap(0).options, " "), "cubeMap forceHighQuality alphaZeroClamp");
    EXPECT_EQ(material->getAllLayers().front()->getFragmentMap(0).map->getExpressionString(), "env/gen1");

    EXPECT_EQ(material->getAllLayers().front()->getFragmentMap(1).index, 1);
    EXPECT_EQ(string::join(material->getAllLayers().front()->getFragmentMap(1).options, " "), "");
    EXPECT_EQ(material->getAllLayers().front()->getFragmentMap(1).map->getExpressionString(), "temp/texture");

    EXPECT_EQ(material->getAllLayers().front()->getFragmentMap(2).index, 2);
    EXPECT_EQ(string::join(material->getAllLayers().front()->getFragmentMap(2).options, " "), "cubemap cameracubemap nearest linear clamp noclamp zeroclamp alphazeroclamp forcehighquality uncompressed highquality nopicmip");
    EXPECT_EQ(material->getAllLayers().front()->getFragmentMap(2).map->getExpressionString(), "temp/optionsftw");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/fragmentProgram2");

    EXPECT_EQ(material->getAllLayers().front()->getFragmentProgram(), "glprogs/test.vfp");
    EXPECT_EQ(material->getAllLayers().front()->getNumFragmentMaps(), 3);
    EXPECT_EQ(material->getAllLayers().front()->getFragmentMap(0).index, 0);
    EXPECT_EQ(string::join(material->getAllLayers().front()->getFragmentMap(0).options, " "), "");
    EXPECT_EQ(material->getAllLayers().front()->getFragmentMap(0).map->getExpressionString(), "env/gen1");

    EXPECT_EQ(material->getAllLayers().front()->getFragmentMap(1).index, -1); // is missing
    EXPECT_EQ(string::join(material->getAllLayers().front()->getFragmentMap(1).options, " "), "");
    EXPECT_FALSE(material->getAllLayers().front()->getFragmentMap(1).map);

    EXPECT_EQ(material->getAllLayers().front()->getFragmentMap(2).index, 2);
    EXPECT_EQ(string::join(material->getAllLayers().front()->getFragmentMap(2).options, " "), "");
    EXPECT_EQ(material->getAllLayers().front()->getFragmentMap(2).map->getExpressionString(), "temp/texture");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/fragmentProgram3");
    EXPECT_TRUE(material->getAllLayers().empty()); // failure to parse should end up with an empty material
}

TEST_F(MaterialsTest, MaterialParserGuiSurf)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/guisurf1");
    EXPECT_TRUE(material->getSurfaceFlags() & Material::SURF_GUISURF);
    EXPECT_FALSE(material->getSurfaceFlags() & (Material::SURF_ENTITYGUI| Material::SURF_ENTITYGUI2| Material::SURF_ENTITYGUI3));
    EXPECT_EQ(material->getGuiSurfArgument(), "guis/lvlmaps/genericmap.gui");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/guisurf2");
    EXPECT_TRUE(material->getSurfaceFlags() & Material::SURF_GUISURF);
    EXPECT_TRUE(material->getSurfaceFlags() & Material::SURF_ENTITYGUI);
    EXPECT_FALSE(material->getSurfaceFlags() & (Material::SURF_ENTITYGUI2 | Material::SURF_ENTITYGUI3));
    EXPECT_EQ(material->getGuiSurfArgument(), "");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/guisurf3");
    EXPECT_TRUE(material->getSurfaceFlags() & Material::SURF_GUISURF);
    EXPECT_TRUE(material->getSurfaceFlags() & Material::SURF_ENTITYGUI2);
    EXPECT_FALSE(material->getSurfaceFlags() & (Material::SURF_ENTITYGUI | Material::SURF_ENTITYGUI3));
    EXPECT_EQ(material->getGuiSurfArgument(), "");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/guisurf4");
    EXPECT_TRUE(material->getSurfaceFlags() & Material::SURF_GUISURF);
    EXPECT_TRUE(material->getSurfaceFlags() & Material::SURF_ENTITYGUI3);
    EXPECT_FALSE(material->getSurfaceFlags() & (Material::SURF_ENTITYGUI | Material::SURF_ENTITYGUI2));
    EXPECT_EQ(material->getGuiSurfArgument(), "");
}

}
