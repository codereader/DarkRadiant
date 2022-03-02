#include "RadiantTest.h"

#include "ishaders.h"
#include <algorithm>
#include "string/split.h"
#include "string/case_conv.h"
#include "string/trim.h"
#include "string/join.h"
#include "math/MatrixUtils.h"
#include "materials/FrobStageSetup.h"

namespace test
{

using MaterialsTest = RadiantTest;

constexpr double TestEpsilon = 0.0001;

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

TEST_F(MaterialsTest, MaterialCanBeModified)
{
    auto& materialManager = GlobalMaterialManager();

    EXPECT_TRUE(materialManager.materialCanBeModified("textures/numbers/0"));
    EXPECT_FALSE(materialManager.materialCanBeModified("textures/AFX/AFXweight"));
}

TEST_F(MaterialsTest, MaterialCreation)
{
    auto& materialManager = GlobalMaterialManager();

    std::string firedName;

    materialManager.signal_materialCreated().connect([&](const std::string& name)
    {
        firedName = name;
    });

    auto material = materialManager.createEmptyMaterial("textures/test/doesnotexistyet");
    EXPECT_TRUE(material);
    EXPECT_EQ(material->getName(), "textures/test/doesnotexistyet");
    EXPECT_TRUE(material->isModified()); // new material should be marked as modified
    EXPECT_TRUE(materialManager.materialCanBeModified(material->getName()));

    // Check that the signal got emitted
    EXPECT_NE(firedName, "");
    EXPECT_EQ(firedName, material->getName());

    firedName.clear();

    // Attempting to use the same name again will not succeed
    material = materialManager.createEmptyMaterial("textures/test/doesnotexistyet");
    EXPECT_TRUE(material);
    EXPECT_EQ(material->getName(), "textures/test/doesnotexistyet01");

    // Check that the signal got emitted
    EXPECT_NE(firedName, "");
    EXPECT_EQ(firedName, material->getName());
}

TEST_F(MaterialsTest, MaterialRenaming)
{
    auto& materialManager = GlobalMaterialManager();

    std::string firedOldName;
    std::string firedNewName;

    materialManager.signal_materialRenamed().connect([&](const std::string& oldName, const std::string& newName)
    {
        firedOldName = oldName;
        firedNewName = newName;
    });

    auto material = materialManager.createEmptyMaterial("textures/test/firstname");
    EXPECT_TRUE(material);
    EXPECT_EQ(material->getName(), "textures/test/firstname");

    // Rename the first material
    EXPECT_TRUE(materialManager.renameMaterial("textures/test/firstname", "textures/test/anothername"));

    // The material reference needs to be renamed too
    EXPECT_EQ(material->getName(), "textures/test/anothername");
    
    // Renamed material should still be marked as modified
    EXPECT_TRUE(material->isModified());

    // Re-acquiring the material reference should also deliver the same modified instance
    material = materialManager.getMaterial("textures/test/anothername");
    EXPECT_TRUE(material->isModified());

    // Check signal emission
    EXPECT_EQ(firedOldName, "textures/test/firstname");
    EXPECT_EQ(firedNewName, "textures/test/anothername");

    firedOldName.clear();
    firedNewName.clear();

    // Cannot rename a non-existent material
    EXPECT_FALSE(materialManager.renameMaterial("textures/test/firstname", "whatevername"));

    // Check signal emission
    EXPECT_EQ(firedOldName, "");
    EXPECT_EQ(firedNewName, "");

    // Create a new material
    materialManager.createEmptyMaterial("textures/test/secondname");

    // Cannot rename a material to a conflicting name
    EXPECT_FALSE(materialManager.renameMaterial("textures/test/secondname", "textures/test/anothername"));

    // Check signal emission
    EXPECT_EQ(firedOldName, "");
    EXPECT_EQ(firedNewName, "");

    // Cannot rename a material to the same name
    EXPECT_FALSE(materialManager.renameMaterial("textures/test/secondname", "textures/test/secondname"));

    // Check signal emission
    EXPECT_EQ(firedOldName, "");
    EXPECT_EQ(firedNewName, "");
}

TEST_F(MaterialsTest, MaterialCopy)
{
    auto& materialManager = GlobalMaterialManager();

    std::string firedNewName;

    materialManager.signal_materialCreated().connect([&](const std::string& newName)
    {
        firedNewName = newName;
    });

    EXPECT_TRUE(materialManager.materialExists("textures/AFX/AFXweight"));

    // Copy name must not be empty => returns empty material
    EXPECT_FALSE(materialManager.copyMaterial("textures/AFX/AFXweight", ""));

    // Source material name must be existent
    EXPECT_FALSE(materialManager.copyMaterial("textures/menotexist", "texures/copytest"));

    auto material = materialManager.copyMaterial("textures/AFX/AFXweight", "texures/copytest");
    EXPECT_TRUE(material);
    EXPECT_EQ(material->getName(), "texures/copytest");
    EXPECT_TRUE(materialManager.materialCanBeModified("texures/copytest"));
    EXPECT_STREQ(material->getShaderFileName(), "");
    EXPECT_EQ(material->getShaderFileInfo().name, "");
    EXPECT_EQ(material->getShaderFileInfo().topDir, "");

    // Check signal emission
    EXPECT_EQ(firedNewName, "texures/copytest");
}

TEST_F(MaterialsTest, MaterialRemoval)
{
    auto& materialManager = GlobalMaterialManager();

    std::string firedName;

    materialManager.signal_materialRemoved().connect([&](const std::string& name)
    {
        firedName = name;
    });

    auto material = materialManager.createEmptyMaterial("textures/test/firstname");
    EXPECT_TRUE(material);
    EXPECT_EQ(material->getName(), "textures/test/firstname");

    materialManager.removeMaterial("textures/test/firstname");
    EXPECT_FALSE(materialManager.materialExists("textures/test/firstname"));

    // Check signal emission
    EXPECT_EQ(firedName, "textures/test/firstname");

    firedName.clear();

    // Removing a non-existent material is not firing any signals
    materialManager.removeMaterial("textures/test/firstname");

    EXPECT_EQ(firedName, "");
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

void performLookupTests(const ITableDefinition::Ptr& table, const std::vector<std::pair<float, float>>& testCases)
{
    for (auto testcase : testCases)
    {
        EXPECT_NEAR(table->getValue(testcase.first), testcase.second, TestEpsilon) << "Lookup failed: "
            << table->getName() << "[" << testcase.first << "] = " << table->getValue(testcase.first) << ", but should be " << testcase.second;
    }
}

TEST_F(MaterialsTest, MaterialTableLookup)
{
    auto table = GlobalMaterialManager().getTable("sinTable");

    std::vector<std::pair<float, float>> testCases
    {
      {  -9.400000f, -0.587745f },
      {  -1.000000f,  0.000000f },
      {  -0.355500f, -0.788223f },
      {  -0.000025f, -0.000157f },
      {   0.000000f,  0.000000f },
      {   0.000025f,  0.000157f },
      {   0.050000f,  0.309003f },
      {   0.250000f,  1.000000f },
      {   0.332200f,  0.869553f },
      {   0.700020f, -0.951048f },
      {   0.999980f, -0.000126f },
      {   1.000000f,  0.000000f },
      {   1.002000f,  0.012565f },
      {   1.800000f, -0.951010f },
      {   2.300000f,  0.951010f },
      {  60.500000f,  0.000000f },
      { 100.230003f,  0.992086f }
    };

    performLookupTests(table, testCases);
}

TEST_F(MaterialsTest, MaterialTableLookupSnapped)
{
    auto table = GlobalMaterialManager().getTable("snapTest");

    std::vector<std::pair<float, float>> testCases
    {
      {  -9.400000f, 1.000000f },
      {  -1.000000f, 1.000000f },
      {  -0.355500f, 0.000000f },
      {  -0.000025f, 0.000000f },
      {   0.000000f, 1.000000f },
      {   0.000025f, 1.000000f },
      {   0.050000f, 0.000000f },
      {   0.250000f, 0.000000f },
      {   0.332200f, 1.000000f },
      {   0.400000f, 1.000000f },
      {   0.430000f, 1.000000f },
      {   0.450000f, 1.000000f },
      {   0.460000f, 1.000000f },
      {   0.490000f, 0.000000f },
      {   0.700020f, 0.000000f },
      {   0.910000f, 0.000000f },
      {   0.999980f, 0.000000f },
      {   1.000000f, 1.000000f },
      {   1.002000f, 1.000000f },
      {   1.800000f, 0.000000f },
      {   2.300000f, 1.000000f },
      {  60.500000f, 0.000000f },
      { 100.230003f, 0.000000f },
    };

    performLookupTests(table, testCases);
}

TEST_F(MaterialsTest, MaterialTableLookupClamped)
{
    auto table = GlobalMaterialManager().getTable("clampTest");

    std::vector<std::pair<float, float>> testCases
    {
        {  -9.400000f, 1.000000f },
        {  -1.000000f, 1.000000f },
        {  -0.355500f, 1.000000f },
        {  -0.000025f, 1.000000f },
        {   0.000000f, 1.000000f },
        {   0.000025f, 1.000000f },
        {   0.050000f, 1.000000f },
        {   0.250000f, 1.000000f },
        {   0.332200f, 1.000000f },
        {   0.700020f, 1.000000f },
        {   0.910000f, 0.809999f },
        {   0.999980f, 0.000180f },
        {   1.000000f, 0.000000f },
        {   1.002000f, 0.000000f },
        {   1.800000f, 0.000000f },
        {   2.300000f, 0.000000f },
        {  60.500000f, 0.000000f },
        { 100.230003f, 0.000000f },
    };

    performLookupTests(table, testCases);
}

TEST_F(MaterialsTest, MaterialRotationEvaluation)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/expressions/rotationCalculation");

    auto& stage = material->getAllLayers().front();

    // Set time to 5008 seconds, this is the value I happened to run into when debugging this in the engine
    stage->evaluateExpressions(5008);

    auto expectedMatrix = Matrix4::byRows(
        0.999998033,    0.000158024632, 0,  0,
        -0.000158024632, 0.999998033, 0, 0,
        0,              0,              1,  0,
        0,              0,              0,  1
    );
    expectNear(stage->getTextureTransform(), expectedMatrix);
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

    material = materialManager.getMaterial("textures/parsertest/sort_custom");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_FLOAT_EQ(material->getSortRequest(), 34.56f);

    material = materialManager.getMaterial("textures/parsertest/sort_custom2");
    EXPECT_TRUE(material->getParseFlags() & Material::PF_HasSortDefined);
    EXPECT_FLOAT_EQ(material->getSortRequest(), 34.56f);

    // Update the sort request
    material->setSortRequest(78.45f);
    EXPECT_FLOAT_EQ(material->getSortRequest(), 78.45f);
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

TEST_F(MaterialsTest, MaterialParserStageNotransform)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/notransform");
    auto stage = material->getAllLayers().front();

    EXPECT_EQ(stage->getTransformations().size(), 0);
    EXPECT_TRUE(stage->getTextureTransform() == Matrix4::getIdentity());
}

TEST_F(MaterialsTest, MaterialParserStageTranslate)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/translation1");
    auto stage = material->getAllLayers().front();

    EXPECT_EQ(stage->getTransformations().size(), 1);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::Translate);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "3.0");
    EXPECT_EQ(stage->getTransformations().at(0).expression2->getExpressionString(), "parm3 + 5.0");

    stage->evaluateExpressions(1000);
    expectNear(stage->getTextureTransform(), Matrix4::getTranslation(Vector3(3.0, 5.0, 0)));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/translation2");
    stage = material->getAllLayers().front();

    EXPECT_EQ(stage->getTransformations().size(), 1);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::Translate);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "time");
    EXPECT_EQ(stage->getTransformations().at(0).expression2->getExpressionString(), "0.5");

    stage->evaluateExpressions(1000);
    expectNear(stage->getTextureTransform(), Matrix4::getTranslation(Vector3(1.0, 0.5, 0)));
}

TEST_F(MaterialsTest, MaterialParserStageRotate)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/rotate1");
    auto stage = material->getAllLayers().front();

    EXPECT_EQ(stage->getTransformations().size(), 1);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::Rotate);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "0.03");
    EXPECT_FALSE(stage->getTransformations().at(0).expression2);

    // sintable and costable lookups are [0..1], translate them to [0..2pi]
    auto cosValue = cos(0.03 * 2 * math::PI);
    auto sinValue = sin(0.03 * 2 * math::PI);

    stage->evaluateExpressions(1000);
    expectNear(stage->getTextureTransform(), Matrix4::byRows(
        cosValue, -sinValue, 0, (-0.5*cosValue + 0.5*sinValue) + 0.5,
        sinValue,  cosValue, 0, (-0.5*sinValue - 0.5*cosValue) + 0.5,
        0, 0, 1, 0,
        0, 0, 0, 1
    ));
}

TEST_F(MaterialsTest, MaterialParserStageScale)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/scale1");
    auto stage = material->getAllLayers().front();

    EXPECT_EQ(stage->getTransformations().size(), 1);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::Scale);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "4.0");
    EXPECT_EQ(stage->getTransformations().at(0).expression2->getExpressionString(), "time * 3.0");

    stage->evaluateExpressions(1000);
    expectNear(stage->getTextureTransform(), Matrix4::byRows(
        4, 0, 0, 0,
        0, 3, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    ));
}

TEST_F(MaterialsTest, MaterialParserStageCenterScale)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/centerscale1");
    auto stage = material->getAllLayers().front();

    EXPECT_EQ(stage->getTransformations().size(), 1);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::CenterScale);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "4.0");
    EXPECT_EQ(stage->getTransformations().at(0).expression2->getExpressionString(), "time * 3.0");

    stage->evaluateExpressions(1000);
    expectNear(stage->getTextureTransform(), Matrix4::byRows(
        4, 0, 0, 0.5 - 0.5 * 4,
        0, 3, 0, 0.5 - 0.5 * 3,
        0, 0, 1, 0,
        0, 0, 0, 1
    ));
}

TEST_F(MaterialsTest, MaterialParserStageShear)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/shear1");
    auto stage = material->getAllLayers().front();
    EXPECT_EQ(stage->getTransformations().size(), 1);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::Shear);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "global3 + 5.0");
    EXPECT_EQ(stage->getTransformations().at(0).expression2->getExpressionString(), "4.0");

    stage->evaluateExpressions(1000);
    expectNear(stage->getTextureTransform(), Matrix4::byRows(
        1,  5,  0,  -0.5 * 5,
        4,  1,  0,  -0.5 * 4,
        0,  0,  1,   0,
        0,  0,  0,   1
    ));
}

TEST_F(MaterialsTest, MaterialParserStageTransforms)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/combined1");

    auto stage = material->getAllLayers().front();
    EXPECT_EQ(stage->getTransformations().size(), 2);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::Translate);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "time");
    EXPECT_EQ(stage->getTransformations().at(0).expression2->getExpressionString(), "0.5");
    EXPECT_EQ(stage->getTransformations().at(1).type, IShaderLayer::TransformType::Translate);
    EXPECT_EQ(stage->getTransformations().at(1).expression1->getExpressionString(), "0.7");
    EXPECT_EQ(stage->getTransformations().at(1).expression2->getExpressionString(), "0.5");

    stage->evaluateExpressions(1000);
    expectNear(stage->getTextureTransform(), Matrix4::getTranslation(Vector3(1, 0.5, 0) + Vector3(0.7, 0.5, 0)));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/combined2");

    stage = material->getAllLayers().front();
    EXPECT_EQ(stage->getTransformations().size(), 3);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::Translate);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "time");
    EXPECT_EQ(stage->getTransformations().at(0).expression2->getExpressionString(), "0.5");
    EXPECT_EQ(stage->getTransformations().at(1).type, IShaderLayer::TransformType::Scale);
    EXPECT_EQ(stage->getTransformations().at(1).expression1->getExpressionString(), "0.6");
    EXPECT_EQ(stage->getTransformations().at(1).expression2->getExpressionString(), "0.2");
    EXPECT_EQ(stage->getTransformations().at(2).type, IShaderLayer::TransformType::Translate);
    EXPECT_EQ(stage->getTransformations().at(2).expression1->getExpressionString(), "0.7");
    EXPECT_EQ(stage->getTransformations().at(2).expression2->getExpressionString(), "0.5");

    stage->evaluateExpressions(1000);
    auto combinedMatrix = Matrix4::getTranslation(Vector3(1, 0.5, 0));
    combinedMatrix.premultiplyBy(Matrix4::getScale(Vector3(0.6, 0.2, 1)));
    combinedMatrix.premultiplyBy(Matrix4::getTranslation(Vector3(0.7, 0.5, 0)));
    expectNear(stage->getTextureTransform(), combinedMatrix);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/combined3");

    stage = material->getAllLayers().front();
    EXPECT_EQ(stage->getTransformations().size(), 6);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::Translate);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "time");
    EXPECT_EQ(stage->getTransformations().at(0).expression2->getExpressionString(), "0.5");
    EXPECT_EQ(stage->getTransformations().at(1).type, IShaderLayer::TransformType::Shear);
    EXPECT_EQ(stage->getTransformations().at(1).expression1->getExpressionString(), "0.9");
    EXPECT_EQ(stage->getTransformations().at(1).expression2->getExpressionString(), "0.8");
    EXPECT_EQ(stage->getTransformations().at(2).type, IShaderLayer::TransformType::Rotate);
    EXPECT_EQ(stage->getTransformations().at(2).expression1->getExpressionString(), "0.22");
    EXPECT_FALSE(stage->getTransformations().at(2).expression2);
    EXPECT_EQ(stage->getTransformations().at(3).type, IShaderLayer::TransformType::CenterScale);
    EXPECT_EQ(stage->getTransformations().at(3).expression1->getExpressionString(), "0.2");
    EXPECT_EQ(stage->getTransformations().at(3).expression2->getExpressionString(), "0.1");
    EXPECT_EQ(stage->getTransformations().at(4).type, IShaderLayer::TransformType::Scale);
    EXPECT_EQ(stage->getTransformations().at(4).expression1->getExpressionString(), "0.5");
    EXPECT_EQ(stage->getTransformations().at(4).expression2->getExpressionString(), "0.4");
    EXPECT_EQ(stage->getTransformations().at(5).type, IShaderLayer::TransformType::Translate);
    EXPECT_EQ(stage->getTransformations().at(5).expression1->getExpressionString(), "1.0");
    EXPECT_EQ(stage->getTransformations().at(5).expression2->getExpressionString(), "1.0");

    auto time = 750;
    stage->evaluateExpressions(time);
    auto timeSecs = time / 1000.0;

    combinedMatrix = Matrix4::getTranslation(Vector3(timeSecs, 0.5, 0));

    auto shear = Matrix4::byColumns(1, 0.8, 0, 0, 
                                    0.9, 1.0, 0, 0, 
                                    0, 0, 1, 0, 
                                   -0.5*0.9, -0.5*0.8, 0, 1);
    combinedMatrix.premultiplyBy(shear);

    // sintable and costable lookups are [0..1], translate them to [0..2pi]
    auto cosValue = cos(0.22 * 2 * math::PI);
    auto sinValue = sin(0.22 * 2 * math::PI);

    auto rotate = Matrix4::byRows(cosValue, -sinValue, 0, (-0.5*cosValue+0.5*sinValue) + 0.5,
        sinValue, cosValue, 0, (-0.5*sinValue-0.5*cosValue) + 0.5,
        0, 0, 1, 0,
        0, 0, 0, 1);
    combinedMatrix.premultiplyBy(rotate);

    auto centerScale = Matrix4::byColumns(0.2, 0, 0, 0,
        0, 0.1, 0, 0,
        0, 0, 1, 0,
        0.5 - 0.5*0.2, 0.5 - 0.5*0.1, 0, 1);
    combinedMatrix.premultiplyBy(centerScale);
    combinedMatrix.premultiplyBy(Matrix4::getScale(Vector3(0.5, 0.4, 1)));
    combinedMatrix.premultiplyBy(Matrix4::getTranslation(Vector3(1, 1, 0)));

    expectNear(stage->getTextureTransform(), combinedMatrix);
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

TEST_F(MaterialsTest, MaterialParserRgbaExpressions)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr1");

    auto diffuse = material->getAllLayers().front();
    auto time = 10;
    auto timeSecs = time / 1000.0f; // 0.01
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs*3, 1, 1, 1));
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 3.0");
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr2");

    diffuse = material->getAllLayers().front();
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(1, timeSecs*3, 1, 1));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_RED));
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time * 3.0");
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr3");

    diffuse = material->getAllLayers().front();
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(1, 1, timeSecs * 3, 1));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_RED));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "time * 3.0");
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr4");

    diffuse = material->getAllLayers().front();
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(1, 1, 1, timeSecs * 3));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_RED));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time * 3.0");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr5");

    diffuse = material->getAllLayers().front();
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs * 3, timeSecs * 3, timeSecs * 3, 1));
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 3.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time * 3.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "time * 3.0");
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr6");

    diffuse = material->getAllLayers().front();
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs * 3, timeSecs * 3, timeSecs * 3, timeSecs * 3));
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 3.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time * 3.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "time * 3.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time * 3.0");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr7");

    diffuse = material->getAllLayers().front();
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs * 4, 1, 1, 1)); // second red expression overrules first
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 4.0");
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr8");

    diffuse = material->getAllLayers().front();
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs * 4, timeSecs * 3, timeSecs * 3, 1)); // red overrules rgb
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 4.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time * 3.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "time * 3.0");
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr9");

    diffuse = material->getAllLayers().front();
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs * 3, timeSecs * 4, timeSecs * 3, timeSecs * 3)); // green overrules rgba
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 3.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time * 4.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "time * 3.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time * 3.0");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr10");

    diffuse = material->getAllLayers().front();
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs * 4, timeSecs * 6, timeSecs * 5, timeSecs * 7)); // rgba is overridden
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 4.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time * 6.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "time * 5.0");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time * 7.0");
}

TEST_F(MaterialsTest, MaterialParserLightfallOff)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/lights/lightfalloff1");

    EXPECT_EQ(material->getLightFalloffCubeMapType(), IShaderLayer::MapType::Map);
    EXPECT_EQ(material->getLightFalloffExpression()->getExpressionString(), "makeIntensity(lights/squarelight1a.tga)");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/lights/lightfalloff2");

    EXPECT_EQ(material->getLightFalloffCubeMapType(), IShaderLayer::MapType::CameraCubeMap);
    EXPECT_EQ(material->getLightFalloffExpression()->getExpressionString(), "lights/squarelight1a");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/lights/lightfalloff3");

    // Second lightFallOff declaration overrides the first one in the material
    EXPECT_EQ(material->getLightFalloffCubeMapType(), IShaderLayer::MapType::CameraCubeMap);
    EXPECT_EQ(material->getLightFalloffExpression()->getExpressionString(), "lights/squarelight1a");
}

TEST_F(MaterialsTest, MaterialParserDecalInfo)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/decalinfo");

    EXPECT_EQ(material->getDecalInfo().stayMilliSeconds, 14300);
    EXPECT_EQ(material->getDecalInfo().fadeMilliSeconds, 1500);
    
    EXPECT_NEAR(material->getDecalInfo().startColour.x(), 0.9, TestEpsilon);
    EXPECT_NEAR(material->getDecalInfo().startColour.y(), 0.8, TestEpsilon);
    EXPECT_NEAR(material->getDecalInfo().startColour.z(), 0.7, TestEpsilon);
    EXPECT_NEAR(material->getDecalInfo().startColour.w(), 0.6, TestEpsilon);

    EXPECT_NEAR(material->getDecalInfo().endColour.x(), 0.5, TestEpsilon);
    EXPECT_NEAR(material->getDecalInfo().endColour.y(), 0.5, TestEpsilon);
    EXPECT_NEAR(material->getDecalInfo().endColour.z(), 0.4, TestEpsilon);
    EXPECT_NEAR(material->getDecalInfo().endColour.w(), 0.3, TestEpsilon);
}

TEST_F(MaterialsTest, MaterialParserSurfaceFlags)
{
    constexpr std::pair<const char*, Material::SurfaceFlags> testCases[] =
    {
        { "textures/parsertest/surfaceflags/solid", Material::SURF_SOLID },
        { "textures/parsertest/surfaceflags/water", Material::SURF_WATER },
        { "textures/parsertest/surfaceflags/playerclip", Material::SURF_PLAYERCLIP },
        { "textures/parsertest/surfaceflags/monsterclip", Material::SURF_MONSTERCLIP },
        { "textures/parsertest/surfaceflags/moveableclip", Material::SURF_MOVEABLECLIP },
        { "textures/parsertest/surfaceflags/ikclip", Material::SURF_IKCLIP },
        { "textures/parsertest/surfaceflags/blood", Material::SURF_BLOOD },
        { "textures/parsertest/surfaceflags/trigger", Material::SURF_TRIGGER },
        { "textures/parsertest/surfaceflags/aassolid", Material::SURF_AASSOLID },
        { "textures/parsertest/surfaceflags/aasobstacle", Material::SURF_AASOBSTACLE },
        { "textures/parsertest/surfaceflags/flashlight_trigger", Material::SURF_FLASHLIGHT_TRIGGER },
        { "textures/parsertest/surfaceflags/nonsolid", Material::SURF_NONSOLID },
        { "textures/parsertest/surfaceflags/nullnormal", Material::SURF_NULLNORMAL },
        { "textures/parsertest/surfaceflags/areaportal", Material::SURF_AREAPORTAL },
        { "textures/parsertest/surfaceflags/nocarve", Material::SURF_NOCARVE },
        { "textures/parsertest/surfaceflags/discrete", Material::SURF_DISCRETE },
        { "textures/parsertest/surfaceflags/nofragment", Material::SURF_NOFRAGMENT },
        { "textures/parsertest/surfaceflags/slick", Material::SURF_SLICK },
        { "textures/parsertest/surfaceflags/collision", Material::SURF_COLLISION },
        { "textures/parsertest/surfaceflags/noimpact", Material::SURF_NOIMPACT },
        { "textures/parsertest/surfaceflags/nodamage", Material::SURF_NODAMAGE },
        { "textures/parsertest/surfaceflags/ladder", Material::SURF_LADDER },
        { "textures/parsertest/surfaceflags/nosteps", Material::SURF_NOSTEPS },
    };

    for (const auto& testCase : testCases)
    {
        auto material = GlobalMaterialManager().getMaterial(testCase.first);

        EXPECT_EQ((material->getSurfaceFlags() & testCase.second), testCase.second);
    }
}

TEST_F(MaterialsTest, MaterialParserStageTextureFiltering)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/texturefilter/nearest");
    EXPECT_NE(material->getAllLayers().front()->getStageFlags() & IShaderLayer::FLAG_FILTER_NEAREST, 0);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texturefilter/linear");
    EXPECT_NE(material->getAllLayers().front()->getStageFlags() & IShaderLayer::FLAG_FILTER_LINEAR, 0);
}

TEST_F(MaterialsTest, MaterialParserStageTextureQuality)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/texturequality/highquality");
    EXPECT_NE(material->getAllLayers().front()->getStageFlags() & IShaderLayer::FLAG_HIGHQUALITY, 0);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texturequality/uncompressed");
    EXPECT_NE(material->getAllLayers().front()->getStageFlags() & IShaderLayer::FLAG_HIGHQUALITY, 0);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texturequality/forcehighquality");
    EXPECT_NE(material->getAllLayers().front()->getStageFlags() & IShaderLayer::FLAG_FORCE_HIGHQUALITY, 0);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texturequality/nopicmip");
    EXPECT_NE(material->getAllLayers().front()->getStageFlags() & IShaderLayer::FLAG_NO_PICMIP, 0);
}

TEST_F(MaterialsTest, MaterialParserStageTexGen)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/texgen/normal");
    EXPECT_EQ(material->getAllLayers().front()->getTexGenType(), IShaderLayer::TEXGEN_NORMAL);
    EXPECT_FALSE(material->getAllLayers().front()->getTexGenExpression(0));
    EXPECT_FALSE(material->getAllLayers().front()->getTexGenExpression(1));
    EXPECT_FALSE(material->getAllLayers().front()->getTexGenExpression(2));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texgen/reflect");
    EXPECT_EQ(material->getAllLayers().front()->getTexGenType(), IShaderLayer::TEXGEN_REFLECT);
    EXPECT_FALSE(material->getAllLayers().front()->getTexGenExpression(0));
    EXPECT_FALSE(material->getAllLayers().front()->getTexGenExpression(1));
    EXPECT_FALSE(material->getAllLayers().front()->getTexGenExpression(2));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texgen/skybox");
    EXPECT_EQ(material->getAllLayers().front()->getTexGenType(), IShaderLayer::TEXGEN_SKYBOX);
    EXPECT_FALSE(material->getAllLayers().front()->getTexGenExpression(0));
    EXPECT_FALSE(material->getAllLayers().front()->getTexGenExpression(1));
    EXPECT_FALSE(material->getAllLayers().front()->getTexGenExpression(2));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texgen/wobblesky");
    EXPECT_EQ(material->getAllLayers().front()->getTexGenType(), IShaderLayer::TEXGEN_WOBBLESKY);
    EXPECT_EQ(material->getAllLayers().front()->getTexGenExpression(0)->getExpressionString(), "1.0");
    EXPECT_EQ(material->getAllLayers().front()->getTexGenExpression(1)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getAllLayers().front()->getTexGenExpression(2)->getExpressionString(), "(time * 0.6)");
}

TEST_F(MaterialsTest, MaterialParserStageClamp)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/clamping/noclamp");
    EXPECT_EQ(material->getAllLayers().front()->getClampType(), CLAMP_REPEAT);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/clamping/clamp");
    EXPECT_EQ(material->getAllLayers().front()->getClampType(), CLAMP_NOREPEAT);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/clamping/zeroclamp");
    EXPECT_EQ(material->getAllLayers().front()->getClampType(), CLAMP_ZEROCLAMP);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/clamping/alphazeroclamp");
    EXPECT_EQ(material->getAllLayers().front()->getClampType(), CLAMP_ALPHAZEROCLAMP);
}

TEST_F(MaterialsTest, MaterialParserStageFlags)
{
    constexpr std::pair<const char*, int> testCases[] =
    {
        { "textures/parsertest/stageflags/ignorealphatest", IShaderLayer::FLAG_IGNORE_ALPHATEST },
        { "textures/parsertest/stageflags/ignoredepth", IShaderLayer::FLAG_IGNORE_DEPTH },
        { "textures/parsertest/stageflags/maskRed", IShaderLayer::FLAG_MASK_RED },
        { "textures/parsertest/stageflags/maskGreen", IShaderLayer::FLAG_MASK_GREEN },
        { "textures/parsertest/stageflags/maskBlue", IShaderLayer::FLAG_MASK_BLUE },
        { "textures/parsertest/stageflags/maskAlpha", IShaderLayer::FLAG_MASK_ALPHA },
        { "textures/parsertest/stageflags/maskDepth", IShaderLayer::FLAG_MASK_DEPTH },
        { "textures/parsertest/stageflags/maskEverything", 
            (IShaderLayer::FLAG_MASK_RED | IShaderLayer::FLAG_MASK_GREEN |
             IShaderLayer::FLAG_MASK_BLUE | IShaderLayer::FLAG_MASK_ALPHA | IShaderLayer::FLAG_MASK_DEPTH) },
        { "textures/parsertest/stageflags/maskColor",
            (IShaderLayer::FLAG_MASK_RED | IShaderLayer::FLAG_MASK_GREEN | IShaderLayer::FLAG_MASK_BLUE) },
    };

    for (const auto& testCase : testCases)
    {
        auto material = GlobalMaterialManager().getMaterial(testCase.first);
        EXPECT_EQ(material->getAllLayers().front()->getStageFlags() & testCase.second, testCase.second);
    }
}

TEST_F(MaterialsTest, MaterialParserStageVertexColours)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/vertexcolours/none");
    EXPECT_EQ(material->getAllLayers().at(0)->getVertexColourMode(), IShaderLayer::VERTEX_COLOUR_NONE);
    
    material = GlobalMaterialManager().getMaterial("textures/parsertest/vertexcolours/vertexcolour");
    EXPECT_EQ(material->getAllLayers().at(1)->getVertexColourMode(), IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY);
    EXPECT_EQ(material->getAllLayers().at(0)->getVertexColourMode(), IShaderLayer::VERTEX_COLOUR_MULTIPLY);
    
    material = GlobalMaterialManager().getMaterial("textures/parsertest/vertexcolours/colourcomponents");

    // Stage 1: Red
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.5");
    EXPECT_FALSE(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_FALSE(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_FALSE(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_ALPHA));
    EXPECT_FALSE(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 2: Green
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.4");
    EXPECT_FALSE(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_RED));
    EXPECT_FALSE(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_FALSE(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_ALPHA));
    EXPECT_FALSE(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 3: Blue
    EXPECT_EQ(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.3");
    EXPECT_FALSE(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_RED));
    EXPECT_FALSE(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_FALSE(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_ALPHA));
    EXPECT_FALSE(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 4: Alpha
    EXPECT_EQ(material->getAllLayers().at(3)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "0.2");
    EXPECT_FALSE(material->getAllLayers().at(3)->getColourExpression(IShaderLayer::COMP_RED));
    EXPECT_FALSE(material->getAllLayers().at(3)->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_FALSE(material->getAllLayers().at(3)->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_FALSE(material->getAllLayers().at(3)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getAllLayers().at(3)->getColourExpression(IShaderLayer::COMP_RGBA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/vertexcolours/coloured");

    // Stage 1: color expr
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.7");
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.6");
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "0.9");
    EXPECT_FALSE(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 2: colored
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "parm0");
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "parm1");
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "parm2");
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "parm3");
    EXPECT_FALSE(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_RGBA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/vertexcolours/combinations");

    // Stage 1: RGB the same, alpha is different
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time");
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_RGB)->getExpressionString(), "0.5");
    EXPECT_FALSE(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 2: RGBA all the same
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_RGB)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_RGBA)->getExpressionString(), "0.5");

    // Stage 3: RGB overridden by red
    EXPECT_EQ(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.4");
    EXPECT_EQ(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.3");
    EXPECT_EQ(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.3");
    EXPECT_FALSE(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_ALPHA));
    EXPECT_FALSE(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 4: RGBA overridden by alpha
    EXPECT_EQ(material->getAllLayers().at(3)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.2");
    EXPECT_EQ(material->getAllLayers().at(3)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.2");
    EXPECT_EQ(material->getAllLayers().at(3)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.2");
    EXPECT_EQ(material->getAllLayers().at(3)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time");
    EXPECT_EQ(material->getAllLayers().at(3)->getColourExpression(IShaderLayer::COMP_RGB)->getExpressionString(), "0.2");
    EXPECT_FALSE(material->getAllLayers().at(3)->getColourExpression(IShaderLayer::COMP_RGBA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/vertexcolours/combinations2");

    // Stage 1: color overridden by green
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.1");
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time");
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.3");
    EXPECT_EQ(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "0.4");
    EXPECT_FALSE(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getAllLayers().at(0)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 2: color overridden by blue and green such that RGB are equivalent
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.1");
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.1");
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.1");
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "0.4");
    EXPECT_EQ(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_RGB)->getExpressionString(), "0.1");
    EXPECT_FALSE(material->getAllLayers().at(1)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 3: colored overridden by alpha
    EXPECT_EQ(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "parm0");
    EXPECT_EQ(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "parm1");
    EXPECT_EQ(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "parm2");
    EXPECT_EQ(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time");
    EXPECT_FALSE(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getAllLayers().at(2)->getColourExpression(IShaderLayer::COMP_RGBA));
}

TEST_F(MaterialsTest, MaterialParserStagePrivatePolygonOffset)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/notransform");
    EXPECT_EQ(material->getAllLayers().at(0)->getPrivatePolygonOffset(), 0.0f);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/privatePolygonOffset");
    EXPECT_EQ(material->getAllLayers().at(0)->getPrivatePolygonOffset(), -45.9f);
}

TEST_F(MaterialsTest, MaterialParserStageAlphaTest)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/notransform");
    EXPECT_FALSE(material->getAllLayers().at(0)->hasAlphaTest());
    EXPECT_EQ(material->getAllLayers().at(0)->getAlphaTest(), 0.0f);
    
    material = GlobalMaterialManager().getMaterial("textures/parsertest/alphaTest");

    auto layer = material->getAllLayers().at(0);
    layer->evaluateExpressions(0);
    EXPECT_TRUE(layer->hasAlphaTest());
    EXPECT_EQ(layer->getAlphaTest(), 0.0f); // sinTable[0] evaluates to 0.0
    EXPECT_EQ(layer->getAlphaTestExpression()->getExpressionString(), "sinTable[time]");
}

TEST_F(MaterialsTest, MaterialParserStageCondition)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/notransform");
    EXPECT_FALSE(material->getAllLayers().at(0)->getConditionExpression());
    
    material = GlobalMaterialManager().getMaterial("textures/parsertest/condition");

    auto layer = material->getAllLayers().at(0);
    EXPECT_EQ(material->getAllLayers().at(0)->getConditionExpression()->getExpressionString(), "(parm4 > 0.0)");
}

TEST_F(MaterialsTest, MaterialFrobStageDetection)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/frobstage_present1");
    EXPECT_TRUE(shaders::FrobStageSetup::IsPresent(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasWhiteBlendStage(material));
    
    material = GlobalMaterialManager().getMaterial("textures/parsertest/frobstage_present2");
    EXPECT_TRUE(shaders::FrobStageSetup::IsPresent(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasWhiteBlendStage(material));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/frobstage_missing1");
    EXPECT_FALSE(shaders::FrobStageSetup::IsPresent(material));
    EXPECT_FALSE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_FALSE(shaders::FrobStageSetup::HasWhiteBlendStage(material));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/frobstage_missing2");
    EXPECT_FALSE(shaders::FrobStageSetup::IsPresent(material));
    EXPECT_FALSE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasWhiteBlendStage(material));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/frobstage_missing3");
    EXPECT_FALSE(shaders::FrobStageSetup::IsPresent(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_FALSE(shaders::FrobStageSetup::HasWhiteBlendStage(material));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/frobstage_missing4");
    EXPECT_FALSE(shaders::FrobStageSetup::IsPresent(material));
    EXPECT_FALSE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasWhiteBlendStage(material));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/frobstage_missing5");
    EXPECT_FALSE(shaders::FrobStageSetup::IsPresent(material));
    EXPECT_FALSE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasWhiteBlendStage(material));
}

TEST_F(MaterialsTest, MaterialFrobStageAddition)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/frobstage_missing1");
    EXPECT_FALSE(shaders::FrobStageSetup::IsPresent(material));

    shaders::FrobStageSetup::AddToMaterial(material);
    EXPECT_TRUE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasWhiteBlendStage(material));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/frobstage_missing2");
    EXPECT_FALSE(shaders::FrobStageSetup::IsPresent(material));

    shaders::FrobStageSetup::AddToMaterial(material);
    EXPECT_TRUE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasWhiteBlendStage(material));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/frobstage_missing3");
    EXPECT_FALSE(shaders::FrobStageSetup::IsPresent(material));

    shaders::FrobStageSetup::AddToMaterial(material);
    EXPECT_TRUE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasWhiteBlendStage(material));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/frobstage_missing4");
    EXPECT_FALSE(shaders::FrobStageSetup::IsPresent(material));

    // Throws because there's no diffuse
    EXPECT_THROW(shaders::FrobStageSetup::AddToMaterial(material), std::runtime_error);
    EXPECT_FALSE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasWhiteBlendStage(material));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/frobstage_missing5");
    EXPECT_FALSE(shaders::FrobStageSetup::IsPresent(material));

    shaders::FrobStageSetup::AddToMaterial(material);
    EXPECT_TRUE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_TRUE(shaders::FrobStageSetup::HasWhiteBlendStage(material));
}

void checkFrobStageRemoval(const std::string& materialName)
{
    auto material = GlobalMaterialManager().getMaterial(materialName);

    shaders::FrobStageSetup::RemoveFromMaterial(material);
    EXPECT_FALSE(shaders::FrobStageSetup::HasAdditiveDiffuseStage(material));
    EXPECT_FALSE(shaders::FrobStageSetup::HasWhiteBlendStage(material));
}

TEST_F(MaterialsTest, MaterialFrobStageRemoval)
{
    checkFrobStageRemoval("textures/parsertest/frobstage_present1");
    checkFrobStageRemoval("textures/parsertest/frobstage_present2");
    checkFrobStageRemoval("textures/parsertest/frobstage_missing1");
    checkFrobStageRemoval("textures/parsertest/frobstage_missing2");
    checkFrobStageRemoval("textures/parsertest/frobstage_missing3");
    checkFrobStageRemoval("textures/parsertest/frobstage_missing4");
    checkFrobStageRemoval("textures/parsertest/frobstage_missing5");
}

// #5853: Two files define the same material:
// 1) in VFS: materials/z_precedence.mtr
// 2) in PK4: tdm_example_mtrs.pk4/materials/precedence.mtr
// Even though the filesystem folder has higher priority, the file
// in the PK4 should be parsed first, since the lexicographical order
// is considered when looking for .mtr files. That's why the
// declaration in the filesystem should not take effect.
TEST_F(MaterialsTest, MaterialFilenamePrecedence)
{
    auto material = GlobalMaterialManager().getMaterial("textures/precedencecheck");

    EXPECT_TRUE(material) << "Could not find the material textures/precedencecheck";

    // The material in the PK4 should be processed first, the one in the filesystem just produces a warning
    EXPECT_EQ(material->getDescription(), "Defined in precedence.mtr") 
        << "Description does not match what is defined in the PK4 .mtr";
}

}
