#include "RadiantTest.h"

#include "ishaders.h"
#include <algorithm>

#include "string/split.h"
#include "string/case_conv.h"
#include "string/trim.h"
#include "string/join.h"
#include "math/MatrixUtils.h"
#include "materials/FrobStageSetup.h"
#include "testutil/TemporaryFile.h"

namespace test
{

using MaterialsTest = RadiantTest;

constexpr double TestEpsilon = 0.0001;

inline std::vector<IShaderLayer::Ptr> getAllLayers(const MaterialPtr& material)
{
    std::vector<IShaderLayer::Ptr> layers;

    material->foreachLayer([&](const IShaderLayer::Ptr& layer)
    {
        layers.push_back(layer);
        return true;
    });

    return layers;
}

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

TEST_F(MaterialsTest, MaterialRenameSetsModifiedStatus)
{
    auto& materialManager = GlobalMaterialManager();

    auto material = materialManager.getMaterial("textures/numbers/2");
    EXPECT_TRUE(material) << "Cannot find the material textures/numbers/2";
    EXPECT_TRUE(materialManager.materialCanBeModified("textures/numbers/2")) << "Material textures/numbers/2 should be editable";
    EXPECT_FALSE(material->isModified()) << "Unchanged material should report as modified";

    // Rename this material
    EXPECT_TRUE(materialManager.renameMaterial("textures/numbers/2", "textures/changedNumber/2"));
    
    // Renamed material should be marked as modified
    EXPECT_TRUE(material->isModified());
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

    // These are defined in null_byte_at_the_end.mtr which has a 0 character at the bottom of the file (#6108)
    EXPECT_TRUE(materialManager.materialExists("textures/parsertest/something2"));
    EXPECT_TRUE(materialManager.materialExists("textures/parsertest/something3"));
}

TEST_F(MaterialsTest, EnumerateMaterialLayers)
{
    auto material = GlobalMaterialManager().getMaterial("tdm_spider_black");
    EXPECT_TRUE(material);

    // Get a list of all layers in the material
    auto layers = getAllLayers(material);
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
            << table->getDeclName() << "[" << testcase.first << "] = " << table->getValue(testcase.first) << ", but should be " << testcase.second;
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

    auto stage = material->getLayer(0);

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
    EXPECT_EQ(material->getDeformExpression(0)->getExpressionString(), "(1.7 + time + 4 - global3)");
    EXPECT_FALSE(material->getDeformExpression(1));
    EXPECT_FALSE(material->getDeformExpression(2));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/deform4");
    EXPECT_EQ(material->getDeformType(), Material::DEFORM_TURBULENT);
    EXPECT_EQ(material->getDeformDeclName(), "deformtesttable");
    EXPECT_EQ(material->getDeformExpression(0)->getExpressionString(), "time * 2");
    EXPECT_EQ(material->getDeformExpression(1)->getExpressionString(), "(parm11 - 4)");
    EXPECT_EQ(material->getDeformExpression(2)->getExpressionString(), "-1 * global5");

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
    auto stage = material->getLayer(0);

    EXPECT_EQ(stage->getTransformations().size(), 0);
    EXPECT_TRUE(stage->getTextureTransform() == Matrix4::getIdentity());
}

TEST_F(MaterialsTest, MaterialParserStageTranslate)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/translation1");
    auto stage = material->getLayer(0);

    EXPECT_EQ(stage->getTransformations().size(), 1);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::Translate);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "3");
    EXPECT_EQ(stage->getTransformations().at(0).expression2->getExpressionString(), "parm3 + 5");

    stage->evaluateExpressions(1000);
    // parm3 is the alpha parm which evaluates to 1 by default
    expectNear(stage->getTextureTransform(), Matrix4::getTranslation(Vector3(3.0, 6.0, 0)));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/translation2");
    stage = material->getLayer(0);

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
    auto stage = material->getLayer(0);

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
    auto stage = material->getLayer(0);

    EXPECT_EQ(stage->getTransformations().size(), 1);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::Scale);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "4");
    EXPECT_EQ(stage->getTransformations().at(0).expression2->getExpressionString(), "time * 3");

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
    auto stage = material->getLayer(0);

    EXPECT_EQ(stage->getTransformations().size(), 1);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::CenterScale);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "4");
    EXPECT_EQ(stage->getTransformations().at(0).expression2->getExpressionString(), "time * 3");

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
    auto stage = material->getLayer(0);
    EXPECT_EQ(stage->getTransformations().size(), 1);
    EXPECT_EQ(stage->getTransformations().at(0).type, IShaderLayer::TransformType::Shear);
    EXPECT_EQ(stage->getTransformations().at(0).expression1->getExpressionString(), "global3 + 5");
    EXPECT_EQ(stage->getTransformations().at(0).expression2->getExpressionString(), "4");

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

    auto stage = material->getLayer(0);
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

    stage = material->getLayer(0);
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

    stage = material->getLayer(0);
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
    EXPECT_EQ(stage->getTransformations().at(5).expression1->getExpressionString(), "1");
    EXPECT_EQ(stage->getTransformations().at(5).expression2->getExpressionString(), "1");

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
    material->getLayer(0)->evaluateExpressions(0);

    EXPECT_EQ(material->getLayer(0)->getNumVertexParms(), 1);
    EXPECT_EQ(material->getLayer(0)->getVertexParmValue(0), Vector4(0, 0, 0, 0)); // all 4 equal
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).index, 0);
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[0]->getExpressionString(), "time");
    EXPECT_FALSE(material->getLayer(0)->getVertexParm(0).expressions[1]);
    EXPECT_FALSE(material->getLayer(0)->getVertexParm(0).expressions[2]);
    EXPECT_FALSE(material->getLayer(0)->getVertexParm(0).expressions[3]);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram2");
    material->getLayer(0)->evaluateExpressions(0);

    EXPECT_EQ(material->getLayer(0)->getNumVertexParms(), 1);
    EXPECT_EQ(material->getLayer(0)->getVertexParmValue(0), Vector4(0, 3, 0, 1)); // z=0,w=1 implicitly
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).index, 0);
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[0]->getExpressionString(), "time");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[1]->getExpressionString(), "3");
    EXPECT_FALSE(material->getLayer(0)->getVertexParm(0).expressions[2]);
    EXPECT_FALSE(material->getLayer(0)->getVertexParm(0).expressions[3]);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram3");
    material->getLayer(0)->evaluateExpressions(0);

    EXPECT_EQ(material->getLayer(0)->getNumVertexParms(), 1);
    EXPECT_EQ(material->getLayer(0)->getVertexParmValue(0), Vector4(0, 3, 0, 1)); // w=1 implicitly
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).index, 0);
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[0]->getExpressionString(), "time");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[1]->getExpressionString(), "3");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[2]->getExpressionString(), "global3");
    EXPECT_FALSE(material->getLayer(0)->getVertexParm(0).expressions[3]);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram4");
    material->getLayer(0)->evaluateExpressions(1000); // time = 1 sec

    EXPECT_EQ(material->getLayer(0)->getNumVertexParms(), 1);
    EXPECT_EQ(material->getLayer(0)->getVertexParmValue(0), Vector4(1, 3, 0, 2));
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).index, 0);
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[0]->getExpressionString(), "time");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[1]->getExpressionString(), "3");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[2]->getExpressionString(), "global3");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[3]->getExpressionString(), "time * 2");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram5");
    material->getLayer(0)->evaluateExpressions(2000); // time = 2 secs

    EXPECT_EQ(material->getLayer(0)->getNumVertexParms(), 3);
    EXPECT_EQ(material->getLayer(0)->getVertexParmValue(0), Vector4(2, 3, 0, 4));

    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).index, 0);
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[0]->getExpressionString(), "time");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[1]->getExpressionString(), "3");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[2]->getExpressionString(), "global3");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[3]->getExpressionString(), "time * 2");

    EXPECT_EQ(material->getLayer(0)->getVertexParmValue(1), Vector4(1, 2, 3, 4));
    EXPECT_EQ(material->getLayer(0)->getVertexParm(1).index, 1);
    EXPECT_EQ(material->getLayer(0)->getVertexParm(1).expressions[0]->getExpressionString(), "1");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(1).expressions[1]->getExpressionString(), "2");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(1).expressions[2]->getExpressionString(), "3");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(1).expressions[3]->getExpressionString(), "4");

    EXPECT_EQ(material->getLayer(0)->getVertexParmValue(2), Vector4(5, 6, 7, 8));
    EXPECT_EQ(material->getLayer(0)->getVertexParm(2).index, 2);
    EXPECT_EQ(material->getLayer(0)->getVertexParm(2).expressions[0]->getExpressionString(), "5");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(2).expressions[1]->getExpressionString(), "6");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(2).expressions[2]->getExpressionString(), "7");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(2).expressions[3]->getExpressionString(), "8");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram6");
    material->getLayer(0)->evaluateExpressions(2000); // time = 2 secs

    EXPECT_EQ(material->getLayer(0)->getNumVertexParms(), 3);
    EXPECT_EQ(material->getLayer(0)->getVertexParmValue(0), Vector4(2, 3, 0, 4));

    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).index, 0);
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[0]->getExpressionString(), "time");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[1]->getExpressionString(), "3");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[2]->getExpressionString(), "global3");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(0).expressions[3]->getExpressionString(), "time * 2");

    EXPECT_EQ(material->getLayer(0)->getVertexParmValue(1), Vector4(0, 0, 0, 0));
    EXPECT_EQ(material->getLayer(0)->getVertexParm(1).index, -1); // missing
    EXPECT_FALSE(material->getLayer(0)->getVertexParm(1).expressions[0]);
    EXPECT_FALSE(material->getLayer(0)->getVertexParm(1).expressions[1]);
    EXPECT_FALSE(material->getLayer(0)->getVertexParm(1).expressions[2]);
    EXPECT_FALSE(material->getLayer(0)->getVertexParm(1).expressions[3]);

    EXPECT_EQ(material->getLayer(0)->getVertexParmValue(2), Vector4(5, 6, 7, 8));
    EXPECT_EQ(material->getLayer(0)->getVertexParm(2).index, 2);
    EXPECT_EQ(material->getLayer(0)->getVertexParm(2).expressions[0]->getExpressionString(), "5");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(2).expressions[1]->getExpressionString(), "6");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(2).expressions[2]->getExpressionString(), "7");
    EXPECT_EQ(material->getLayer(0)->getVertexParm(2).expressions[3]->getExpressionString(), "8");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/vertexProgram7");
    EXPECT_EQ(material->getNumLayers(), 0); // failure to parse should end up with an empty material
}

TEST_F(MaterialsTest, MaterialParserStageFragmentProgram)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/program/fragmentProgram1");

    EXPECT_EQ(material->getLayer(0)->getFragmentProgram(), "glprogs/test.vfp");
    EXPECT_EQ(material->getLayer(0)->getNumFragmentMaps(), 3);
    EXPECT_EQ(material->getLayer(0)->getFragmentMap(0).index, 0);
    EXPECT_EQ(string::join(material->getLayer(0)->getFragmentMap(0).options, " "), "cubeMap forceHighQuality alphaZeroClamp");
    EXPECT_EQ(material->getLayer(0)->getFragmentMap(0).map->getExpressionString(), "env/gen1");

    EXPECT_EQ(material->getLayer(0)->getFragmentMap(1).index, 1);
    EXPECT_EQ(string::join(material->getLayer(0)->getFragmentMap(1).options, " "), "");
    EXPECT_EQ(material->getLayer(0)->getFragmentMap(1).map->getExpressionString(), "temp/texture");

    EXPECT_EQ(material->getLayer(0)->getFragmentMap(2).index, 2);
    EXPECT_EQ(string::join(material->getLayer(0)->getFragmentMap(2).options, " "), "cubemap cameracubemap nearest linear clamp noclamp zeroclamp alphazeroclamp forcehighquality uncompressed highquality nopicmip");
    EXPECT_EQ(material->getLayer(0)->getFragmentMap(2).map->getExpressionString(), "temp/optionsftw");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/fragmentProgram2");

    EXPECT_EQ(material->getLayer(0)->getFragmentProgram(), "glprogs/test.vfp");
    EXPECT_EQ(material->getLayer(0)->getNumFragmentMaps(), 3);
    EXPECT_EQ(material->getLayer(0)->getFragmentMap(0).index, 0);
    EXPECT_EQ(string::join(material->getLayer(0)->getFragmentMap(0).options, " "), "");
    EXPECT_EQ(material->getLayer(0)->getFragmentMap(0).map->getExpressionString(), "env/gen1");

    EXPECT_EQ(material->getLayer(0)->getFragmentMap(1).index, -1); // is missing
    EXPECT_EQ(string::join(material->getLayer(0)->getFragmentMap(1).options, " "), "");
    EXPECT_FALSE(material->getLayer(0)->getFragmentMap(1).map);

    EXPECT_EQ(material->getLayer(0)->getFragmentMap(2).index, 2);
    EXPECT_EQ(string::join(material->getLayer(0)->getFragmentMap(2).options, " "), "");
    EXPECT_EQ(material->getLayer(0)->getFragmentMap(2).map->getExpressionString(), "temp/texture");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/program/fragmentProgram3");
    EXPECT_EQ(material->getNumLayers(), 0); // failure to parse should end up with an empty material
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

// Checks the default blend func if no explicit function is defined
TEST_F(MaterialsTest, MaterialParserDefaultBlendFunc)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/defaultBlendFunc");
    auto firstLayer = material->getLayer(0);

    EXPECT_EQ(firstLayer->getType(), IShaderLayer::BLEND);
    EXPECT_EQ(firstLayer->getMapType(), IShaderLayer::MapType::Map);
    EXPECT_TRUE(firstLayer->getMapExpression());
    EXPECT_EQ(firstLayer->getMapExpression()->getExpressionString(), "textures/common/caulk.tga");

    // No explicit blend func means the stage is using "gl_one, gl_zero"
    EXPECT_EQ(firstLayer->getBlendFuncStrings().first, "gl_one");
    EXPECT_EQ(firstLayer->getBlendFuncStrings().second, "gl_zero");
    EXPECT_EQ(firstLayer->getBlendFunc().src, GL_ONE);
    EXPECT_EQ(firstLayer->getBlendFunc().dest, GL_ZERO);
}

TEST_F(MaterialsTest, MaterialParserRgbaExpressions)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr1");

    auto diffuse = material->getLayer(0);
    auto time = 10;
    auto timeSecs = time / 1000.0f; // 0.01
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs*3, 1, 1, 1));
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 3");
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr2");

    diffuse = material->getLayer(0);
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(1, timeSecs*3, 1, 1));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_RED));
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time * 3");
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr3");

    diffuse = material->getLayer(0);
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(1, 1, timeSecs * 3, 1));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_RED));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "time * 3");
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr4");

    diffuse = material->getLayer(0);
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(1, 1, 1, timeSecs * 3));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_RED));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time * 3");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr5");

    diffuse = material->getLayer(0);
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs * 3, timeSecs * 3, timeSecs * 3, 1));
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 3");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time * 3");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "time * 3");
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr6");

    diffuse = material->getLayer(0);
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs * 3, timeSecs * 3, timeSecs * 3, timeSecs * 3));
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 3");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time * 3");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "time * 3");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time * 3");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr7");

    diffuse = material->getLayer(0);
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs * 4, 1, 1, 1)); // second red expression overrules first
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 4");
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr8");

    diffuse = material->getLayer(0);
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs * 4, timeSecs * 3, timeSecs * 3, 1)); // red overrules rgb
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 4");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time * 3");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "time * 3");
    EXPECT_FALSE(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr9");

    diffuse = material->getLayer(0);
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs * 3, timeSecs * 4, timeSecs * 3, timeSecs * 3)); // green overrules rgba
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 3");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time * 4");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "time * 3");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time * 3");

    material = GlobalMaterialManager().getMaterial("textures/parsertest/colourexpr10");

    diffuse = material->getLayer(0);
    diffuse->evaluateExpressions(time);

    EXPECT_TRUE(diffuse->getColour() == Colour4(timeSecs * 4, timeSecs * 6, timeSecs * 5, timeSecs * 7)); // rgba is overridden
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "time * 4");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time * 6");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "time * 5");
    EXPECT_EQ(diffuse->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time * 7");
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
    EXPECT_NE(material->getLayer(0)->getStageFlags() & IShaderLayer::FLAG_FILTER_NEAREST, 0);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texturefilter/linear");
    EXPECT_NE(material->getLayer(0)->getStageFlags() & IShaderLayer::FLAG_FILTER_LINEAR, 0);
}

TEST_F(MaterialsTest, MaterialParserStageTextureQuality)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/texturequality/highquality");
    EXPECT_NE(material->getLayer(0)->getStageFlags() & IShaderLayer::FLAG_HIGHQUALITY, 0);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texturequality/uncompressed");
    EXPECT_NE(material->getLayer(0)->getStageFlags() & IShaderLayer::FLAG_HIGHQUALITY, 0);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texturequality/forcehighquality");
    EXPECT_NE(material->getLayer(0)->getStageFlags() & IShaderLayer::FLAG_FORCE_HIGHQUALITY, 0);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texturequality/nopicmip");
    EXPECT_NE(material->getLayer(0)->getStageFlags() & IShaderLayer::FLAG_NO_PICMIP, 0);
}

TEST_F(MaterialsTest, MaterialParserStageTexGen)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/texgen/normal");
    EXPECT_EQ(material->getLayer(0)->getTexGenType(), IShaderLayer::TEXGEN_NORMAL);
    EXPECT_FALSE(material->getLayer(0)->getTexGenExpression(0));
    EXPECT_FALSE(material->getLayer(0)->getTexGenExpression(1));
    EXPECT_FALSE(material->getLayer(0)->getTexGenExpression(2));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texgen/reflect");
    EXPECT_EQ(material->getLayer(0)->getTexGenType(), IShaderLayer::TEXGEN_REFLECT);
    EXPECT_FALSE(material->getLayer(0)->getTexGenExpression(0));
    EXPECT_FALSE(material->getLayer(0)->getTexGenExpression(1));
    EXPECT_FALSE(material->getLayer(0)->getTexGenExpression(2));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texgen/skybox");
    EXPECT_EQ(material->getLayer(0)->getTexGenType(), IShaderLayer::TEXGEN_SKYBOX);
    EXPECT_FALSE(material->getLayer(0)->getTexGenExpression(0));
    EXPECT_FALSE(material->getLayer(0)->getTexGenExpression(1));
    EXPECT_FALSE(material->getLayer(0)->getTexGenExpression(2));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/texgen/wobblesky");
    EXPECT_EQ(material->getLayer(0)->getTexGenType(), IShaderLayer::TEXGEN_WOBBLESKY);
    EXPECT_EQ(material->getLayer(0)->getTexGenExpression(0)->getExpressionString(), "1");
    EXPECT_EQ(material->getLayer(0)->getTexGenExpression(1)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getLayer(0)->getTexGenExpression(2)->getExpressionString(), "(time * 0.6)");
}

TEST_F(MaterialsTest, MaterialParserStageClamp)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/clamping/noclamp");
    EXPECT_EQ(material->getLayer(0)->getClampType(), CLAMP_REPEAT);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/clamping/clamp");
    EXPECT_EQ(material->getLayer(0)->getClampType(), CLAMP_NOREPEAT);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/clamping/zeroclamp");
    EXPECT_EQ(material->getLayer(0)->getClampType(), CLAMP_ZEROCLAMP);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/clamping/alphazeroclamp");
    EXPECT_EQ(material->getLayer(0)->getClampType(), CLAMP_ALPHAZEROCLAMP);
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
        EXPECT_EQ(material->getLayer(0)->getStageFlags() & testCase.second, testCase.second);
    }
}

TEST_F(MaterialsTest, MaterialParserStageVertexColours)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/vertexcolours/none");
    EXPECT_EQ(material->getLayer(0)->getVertexColourMode(), IShaderLayer::VERTEX_COLOUR_NONE);
    
    material = GlobalMaterialManager().getMaterial("textures/parsertest/vertexcolours/vertexcolour");
    EXPECT_EQ(material->getLayer(1)->getVertexColourMode(), IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY);
    EXPECT_EQ(material->getLayer(0)->getVertexColourMode(), IShaderLayer::VERTEX_COLOUR_MULTIPLY);
    
    material = GlobalMaterialManager().getMaterial("textures/parsertest/vertexcolours/colourcomponents");

    // Stage 1: Red
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.5");
    EXPECT_FALSE(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_FALSE(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_FALSE(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_ALPHA));
    EXPECT_FALSE(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 2: Green
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.4");
    EXPECT_FALSE(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_RED));
    EXPECT_FALSE(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_FALSE(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_ALPHA));
    EXPECT_FALSE(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 3: Blue
    EXPECT_EQ(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.3");
    EXPECT_FALSE(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_RED));
    EXPECT_FALSE(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_FALSE(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_ALPHA));
    EXPECT_FALSE(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 4: Alpha
    EXPECT_EQ(material->getLayer(3)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "0.2");
    EXPECT_FALSE(material->getLayer(3)->getColourExpression(IShaderLayer::COMP_RED));
    EXPECT_FALSE(material->getLayer(3)->getColourExpression(IShaderLayer::COMP_GREEN));
    EXPECT_FALSE(material->getLayer(3)->getColourExpression(IShaderLayer::COMP_BLUE));
    EXPECT_FALSE(material->getLayer(3)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getLayer(3)->getColourExpression(IShaderLayer::COMP_RGBA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/vertexcolours/coloured");

    // Stage 1: color expr
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.7");
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.6");
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "0.9");
    EXPECT_FALSE(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 2: colored
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "parm0");
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "parm1");
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "parm2");
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "parm3");
    EXPECT_FALSE(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_RGBA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/vertexcolours/combinations");

    // Stage 1: RGB the same, alpha is different
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time");
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_RGB)->getExpressionString(), "0.5");
    EXPECT_FALSE(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 2: RGBA all the same
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_RGB)->getExpressionString(), "0.5");
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_RGBA)->getExpressionString(), "0.5");

    // Stage 3: RGB overridden by red
    EXPECT_EQ(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.4");
    EXPECT_EQ(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.3");
    EXPECT_EQ(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.3");
    EXPECT_FALSE(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_ALPHA));
    EXPECT_FALSE(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 4: RGBA overridden by alpha
    EXPECT_EQ(material->getLayer(3)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.2");
    EXPECT_EQ(material->getLayer(3)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.2");
    EXPECT_EQ(material->getLayer(3)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.2");
    EXPECT_EQ(material->getLayer(3)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time");
    EXPECT_EQ(material->getLayer(3)->getColourExpression(IShaderLayer::COMP_RGB)->getExpressionString(), "0.2");
    EXPECT_FALSE(material->getLayer(3)->getColourExpression(IShaderLayer::COMP_RGBA));

    material = GlobalMaterialManager().getMaterial("textures/parsertest/vertexcolours/combinations2");

    // Stage 1: color overridden by green
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.1");
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "time");
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.3");
    EXPECT_EQ(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "0.4");
    EXPECT_FALSE(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getLayer(0)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 2: color overridden by blue and green such that RGB are equivalent
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "0.1");
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "0.1");
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "0.1");
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "0.4");
    EXPECT_EQ(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_RGB)->getExpressionString(), "0.1");
    EXPECT_FALSE(material->getLayer(1)->getColourExpression(IShaderLayer::COMP_RGBA));

    // Stage 3: colored overridden by alpha
    EXPECT_EQ(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_RED)->getExpressionString(), "parm0");
    EXPECT_EQ(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_GREEN)->getExpressionString(), "parm1");
    EXPECT_EQ(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_BLUE)->getExpressionString(), "parm2");
    EXPECT_EQ(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_ALPHA)->getExpressionString(), "time");
    EXPECT_FALSE(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_RGB));
    EXPECT_FALSE(material->getLayer(2)->getColourExpression(IShaderLayer::COMP_RGBA));
}

TEST_F(MaterialsTest, MaterialParserStagePrivatePolygonOffset)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/notransform");
    EXPECT_EQ(material->getLayer(0)->getPrivatePolygonOffset(), 0.0f);

    material = GlobalMaterialManager().getMaterial("textures/parsertest/privatePolygonOffset");
    EXPECT_EQ(material->getLayer(0)->getPrivatePolygonOffset(), -45.9f);
}

TEST_F(MaterialsTest, MaterialParserStageAlphaTest)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/notransform");
    EXPECT_FALSE(material->getLayer(0)->hasAlphaTest());
    EXPECT_EQ(material->getLayer(0)->getAlphaTest(), 0.0f);
    
    material = GlobalMaterialManager().getMaterial("textures/parsertest/alphaTest");

    auto layer = material->getLayer(0);
    layer->evaluateExpressions(0);
    EXPECT_TRUE(layer->hasAlphaTest());
    EXPECT_EQ(layer->getAlphaTest(), 0.0f); // sinTable[0] evaluates to 0.0
    EXPECT_EQ(layer->getAlphaTestExpression()->getExpressionString(), "sinTable[time]");
}

TEST_F(MaterialsTest, MaterialParserStageCondition)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/transform/notransform");
    EXPECT_FALSE(material->getLayer(0)->getConditionExpression());
    
    material = GlobalMaterialManager().getMaterial("textures/parsertest/condition");

    auto layer = material->getLayer(0);
    EXPECT_EQ(material->getLayer(0)->getConditionExpression()->getExpressionString(), "(parm4 > 0)");
}

TEST_F(MaterialsTest, MaterialParserFrobStageNotspecified)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/frobStage5");

    EXPECT_EQ(material->getFrobStageType(), Material::FrobStageType::Default);
    EXPECT_FALSE(material->getFrobStageMapExpression());
}

TEST_F(MaterialsTest, MaterialParserFrobStageTexture)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/frobStage1");

    EXPECT_EQ(material->getFrobStageType(), Material::FrobStageType::Texture);
    EXPECT_EQ(material->getFrobStageMapExpression()->getExpressionString(), "textures/some/thing");

    // frobstage_texture textures/some/thing 0.15 0.40
    EXPECT_EQ(material->getFrobStageRgbParameter(0), Vector3(0.15, 0.15, 0.15));
    EXPECT_EQ(material->getFrobStageRgbParameter(1), Vector3(0.4, 0.4, 0.4));
}

TEST_F(MaterialsTest, MaterialParserFrobStageTextureRgb)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/frobStage1_rgb");

    EXPECT_EQ(material->getFrobStageType(), Material::FrobStageType::Texture);
    EXPECT_EQ(material->getFrobStageMapExpression()->getExpressionString(), "textures/some/thing");

    // frobstage_texture textures/some/thing (0.25 0.3 0.4) (0.50 0.6 0.7)
    EXPECT_EQ(material->getFrobStageRgbParameter(0), Vector3(0.25, 0.3, 0.4));
    EXPECT_EQ(material->getFrobStageRgbParameter(1), Vector3(0.5, 0.6, 0.7));
}

TEST_F(MaterialsTest, MaterialParserFrobStageDiffuse)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/frobStage2");

    EXPECT_EQ(material->getFrobStageType(), Material::FrobStageType::Diffuse);
    EXPECT_FALSE(material->getFrobStageMapExpression());

    // frobstage_diffuse 0.25 0.50 (no separate RGB factors, the float defines the grey value)
    EXPECT_EQ(material->getFrobStageRgbParameter(0), Vector3(0.25, 0.25, 0.25));
    EXPECT_EQ(material->getFrobStageRgbParameter(1), Vector3(0.5, 0.5, 0.5));
}

TEST_F(MaterialsTest, MaterialParserFrobStageDiffuseRgb)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/frobStage3");

    EXPECT_EQ(material->getFrobStageType(), Material::FrobStageType::Diffuse);
    EXPECT_FALSE(material->getFrobStageMapExpression());

    // frobstage_diffuse (0.25 0.3 0.4) (0.50 0.6 0.7)
    EXPECT_EQ(material->getFrobStageRgbParameter(0), Vector3(0.25, 0.3, 0.4));
    EXPECT_EQ(material->getFrobStageRgbParameter(1), Vector3(0.5, 0.6, 0.7));
}

TEST_F(MaterialsTest, MaterialParserFrobStageNone)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/frobStage4");

    EXPECT_EQ(material->getFrobStageType(), Material::FrobStageType::NoFrobStage);
    EXPECT_FALSE(material->getFrobStageMapExpression());
}

TEST_F(MaterialsTest, MaterialDefaultFrobStageSetup)
{
    auto material = GlobalMaterialManager().createEmptyMaterial("dummy_material");

    EXPECT_EQ(material->getFrobStageType(), Material::FrobStageType::Default);
    EXPECT_FALSE(material->getFrobStageMapExpression());
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

TEST_F(MaterialsTest, CoverageOfMaterialWithBlendStage)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/coverage1");

    EXPECT_TRUE(material) << "Could not find the material textures/parsertest/coverage1";
    EXPECT_EQ(material->getCoverage(), Material::MC_OPAQUE) << "Material should be opaque";
}

TEST_F(MaterialsTest, CoverageOfTranslucentMaterial)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/coverage2");

    EXPECT_TRUE(material) << "Could not find the material textures/parsertest/coverage2";
    EXPECT_EQ(material->getCoverage(), Material::MC_TRANSLUCENT) << "Material should be translucent";
}

TEST_F(MaterialsTest, ParseNoShadowsFlag)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/noshadowsflag");

    EXPECT_TRUE(material) << "Could not find the material textures/parsertest/coverage1";
    EXPECT_EQ(material->getMaterialFlags() & Material::FLAG_NOSHADOWS, Material::FLAG_NOSHADOWS) << "Material should have noshadows set";
}

TEST_F(MaterialsTest, MaterialParserRemoteRenderMap)
{
    // Remote Render Map without map expression
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/remoteRenderMap1");
    EXPECT_TRUE(material) << "Could not find the material textures/parsertest/remoteRenderMap1";

    auto layers = getAllLayers(material);

    EXPECT_EQ(layers.at(0)->getType(), IShaderLayer::BLEND);
    EXPECT_EQ(layers.at(0)->getMapType(), IShaderLayer::MapType::RemoteRenderMap);
    EXPECT_EQ(layers.at(0)->getRenderMapSize(), Vector2(232, 232));
    EXPECT_FALSE(layers.at(0)->getMapExpression());

    // Remote Render Map with map expression
    material = GlobalMaterialManager().getMaterial("textures/parsertest/remoteRenderMap2");
    EXPECT_TRUE(material) << "Could not find the material textures/parsertest/remoteRenderMap2";

    layers = getAllLayers(material);

    EXPECT_EQ(layers.at(0)->getType(), IShaderLayer::BLEND);
    EXPECT_EQ(layers.at(0)->getMapType(), IShaderLayer::MapType::RemoteRenderMap);
    EXPECT_EQ(layers.at(0)->getRenderMapSize(), Vector2(256, 128));
    EXPECT_TRUE(layers.at(0)->getMapExpression());
    EXPECT_EQ(layers.at(0)->getMapExpression()->getExpressionString(), "textures/common/mirror.tga");
}

TEST_F(MaterialsTest, MaterialParserMirrorRenderMap)
{
    // Mirror Render Map without map expression
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/mirrorRenderMap1");
    EXPECT_TRUE(material) << "Could not find the material textures/parsertest/mirrorRenderMap1";

    auto layers = getAllLayers(material);

    EXPECT_EQ(layers.at(0)->getType(), IShaderLayer::BLEND);
    EXPECT_EQ(layers.at(0)->getMapType(), IShaderLayer::MapType::MirrorRenderMap);
    EXPECT_EQ(layers.at(0)->getRenderMapSize(), Vector2(256, 128));
    EXPECT_FALSE(layers.at(0)->getMapExpression());

    // Mirror Render Map with map expression
    material = GlobalMaterialManager().getMaterial("textures/parsertest/mirrorRenderMap2");
    EXPECT_TRUE(material) << "Could not find the material textures/parsertest/mirrorRenderMap2";

    layers = getAllLayers(material);

    EXPECT_EQ(layers.at(0)->getType(), IShaderLayer::BLEND);
    EXPECT_EQ(layers.at(0)->getMapType(), IShaderLayer::MapType::MirrorRenderMap);
    EXPECT_EQ(layers.at(0)->getRenderMapSize(), Vector2(256, 128));
    EXPECT_TRUE(layers.at(0)->getMapExpression());
    EXPECT_EQ(layers.at(0)->getMapExpression()->getExpressionString(), "textures/common/mirror.tga");

    // Mirror Render Map without dimensions
    material = GlobalMaterialManager().getMaterial("textures/parsertest/mirrorRenderMap3");
    EXPECT_TRUE(material) << "Could not find the material textures/parsertest/mirrorRenderMap3";

    layers = getAllLayers(material);

    EXPECT_EQ(layers.at(0)->getType(), IShaderLayer::BLEND);
    EXPECT_EQ(layers.at(0)->getMapType(), IShaderLayer::MapType::MirrorRenderMap);
    EXPECT_EQ(layers.at(0)->getRenderMapSize(), Vector2(0, 0));
    EXPECT_TRUE(layers.at(0)->getMapExpression());
    EXPECT_EQ(layers.at(0)->getMapExpression()->getExpressionString(), "textures/common/mirror.tga");
}

TEST_F(MaterialsTest, ShaderExpressionEvaluation)
{
    constexpr std::size_t TimeInMilliseconds = 200;
    constexpr float TimeInSeconds = TimeInMilliseconds / 1000.0f;
    auto testExpressions = std::map<std::string, float>
    {
        { "3", 3.0f },
        { "3+4", 7.0f },
        { "(3+4)", 7.0f },
        { "(4.2)", 4.2f },
        { "3+5+6", 14.0f },
        { "3+(5+6)", 14.0f },
        { "3 * 3+5", 14.0f },
        { "3+3*5", 18.0f },
        { "(3+3)*5", 30.0f },
        { "(3+3*7)-5", 19.0f },
        { "3-3*5", -12.0f },
        { "cosTable[0]", 1.0f },
        { "cosTable[1]", 1.0f },
        { "cosTable[time*5]", 1.0f },
        { "cosTable[0.3]", -0.3090f },
        { "3-3*cosTable[2]", 0.0f },
        { "3+cosTable[3]*7", 10.0f },
        { "(3+cosTable[3])*7", 28.0f },
        { "2.3 % 2", 0.3f },
        { "2.0 % 0.5", 0.0f },
        { "2 == 2", 1.0f },
        { "1 == 2", 0.0f },
        { "1 != 2", 1.0f },
        { "1.2 != 1.2", 0.0f },
        { "1.2 == 1.2*3", 0.0f },
        { "1.2*3 == 1.2*3", 1.0f },
        { "3 == 3 && 1 != 0", 1.0f },
        { "1 != 1 || 3 == 3", 1.0f },
        { "4 == 3 || 1 != 0", 1.0f },
        { "time", TimeInSeconds }, // time is measured in seconds
        { "-3 + 5", 2.0f },
        { "3 * -5", -15.0f },
        { "3 * -5 + 4", -11.0f },
        { "3 + -5 * 4", -17.0f },
        { "3 * 5 * -6", -90.0f },
        { "time / 6 * 3", TimeInSeconds / 6.0f * 3.0f },
        { "time / (6 * 3)", TimeInSeconds / 18.0f },
        { "(time / 6) * 3", (TimeInSeconds / 6.0f) * 3.0f },
        { "9 - 5 + 2", 6.0f },
        { "9 - 5 - 2", 2.0f },
        { "0 && 1 || 1", 1.0f },
        { "1 || 1 && 0", 1.0f },
        { "0 <= 1 <= 0", 0.0f },
        { "5 >= 7 == 7 >= 9", 1.0f },
    };

    for (const auto& [expressionString, expectedValue] : testExpressions)
    {
        auto expr = GlobalMaterialManager().createShaderExpressionFromString(expressionString);

        EXPECT_NEAR(expr->getValue(TimeInMilliseconds), expectedValue, 0.001f) <<
            "Expression " << expressionString << " should evaluate to " << expectedValue;
    }
}

TEST_F(MaterialsTest, UpdateFromValidSourceText)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");
    EXPECT_TRUE(material) << "Could not find the material textures/exporttest/empty";

    auto sourceText = R"(
    diffusemap _white
    {
        blend blend
        map _flat
        rgb 0.5
    }
)";

    auto result = material->updateFromSourceText(sourceText);

    EXPECT_TRUE(result.success) << "Update from source text should have been succeeded";

    EXPECT_EQ(material->getNumLayers(), 2);
    EXPECT_EQ(material->getLayer(0)->getMapExpression()->getExpressionString(), "_white");
}

TEST_F(MaterialsTest, UpdateFromValidSourceTextEmitsSignal)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");
    EXPECT_TRUE(material) << "Could not find the material textures/exporttest/empty";
    
    auto changedSignalCount = 0;

    material->sig_materialChanged().connect(
        [&]() { changedSignalCount++; }
    );

    auto result = material->updateFromSourceText(R"(diffusemap _white)");

    EXPECT_TRUE(result.success) << "Update from source text should have been succeeded";
    EXPECT_EQ(changedSignalCount, 1) << "Changed signal should have been fired exactly once";
}

// Failure to parse the source text should not affect the material
TEST_F(MaterialsTest, UpdateFromInvalidSourceText)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/expressions/sinTableLookup");
    EXPECT_TRUE(material) << "Could not find the material textures/parsertest/expressions/sinTableLookup";

    EXPECT_EQ(material->getNumLayers(), 1) << "Material should have one layer";
    EXPECT_EQ(material->getLayer(0)->getAlphaTestExpression()->getExpressionString(), "sinTable[0.5008]") << "Alphatest expression is missing";

    auto changedSignalCount = 0;

    material->sig_materialChanged().connect(
        [&]() { changedSignalCount++; }
    );

    // The material parser is very tolerant against errors, but an improper vertex parm index does the trick
    auto result = material->updateFromSourceText(R"( {
        map _white
        vertexProgram glprogs/test.vfp
        vertexParm -3 time // invalid vertex parm
    })");

    EXPECT_FALSE(result.success) << "Update from source text should have been succeeded";
    EXPECT_EQ(result.parseError, "A material stage can have 4 vertex parameters at most") << "Wrong error message";

    EXPECT_EQ(material->getNumLayers(), 1) << "Material should still have one layer";
    EXPECT_EQ(material->getLayer(0)->getAlphaTestExpression()->getExpressionString(), "sinTable[0.5008]") << "Alphatest expression got lost";

    EXPECT_EQ(changedSignalCount, 0) << "No changed signal should have been fired";
}

TEST_F(MaterialsTest, ChangingTranslucentFlagSetsCoverage)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/coverage2");

    EXPECT_TRUE(material) << "Could not find the material textures/parsertest/coverage2";
    EXPECT_EQ(material->getCoverage(), Material::MC_TRANSLUCENT) << "Material should be translucent";

    material->clearMaterialFlag(Material::FLAG_TRANSLUCENT);
    EXPECT_EQ(material->getCoverage(), Material::MC_OPAQUE) << "Material should be opaque now";

    material->setMaterialFlag(Material::FLAG_TRANSLUCENT);
    EXPECT_EQ(material->getCoverage(), Material::MC_TRANSLUCENT) << "Material should be translucent again";
}

TEST_F(MaterialsTest, ChangingTranslucentFlagAffectsNoShadows)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/coverage2");

    EXPECT_TRUE(material) << "Could not find the material textures/parsertest/coverage2";
    EXPECT_EQ(material->getCoverage(), Material::MC_TRANSLUCENT) << "Material should be translucent";
    EXPECT_EQ(material->surfaceCastsShadow(), false) << "Material should not cast shadows";
    EXPECT_TRUE(material->getMaterialFlags() & Material::FLAG_NOSHADOWS) << "Material should have the noshadows flag set";

    material->clearMaterialFlag(Material::FLAG_TRANSLUCENT);
    material->clearMaterialFlag(Material::FLAG_NOSHADOWS); // clearing translucent doesn't remove noshadows
    EXPECT_EQ(material->getCoverage(), Material::MC_OPAQUE) << "Material should be opaque now";
    EXPECT_FALSE(material->getMaterialFlags() & Material::FLAG_NOSHADOWS) << "Material should not have the noshadows flag set";
    EXPECT_EQ(material->surfaceCastsShadow(), true) << "Material should cast shadows now";

    material->setMaterialFlag(Material::FLAG_TRANSLUCENT);
    EXPECT_EQ(material->getCoverage(), Material::MC_TRANSLUCENT) << "Material should be translucent again";
    EXPECT_EQ(material->surfaceCastsShadow(), false) << "Material should not cast shadows anymore";
    EXPECT_TRUE(material->getMaterialFlags() & Material::FLAG_NOSHADOWS) << "Material should have the noshadows flag set automatically";
}

TEST_F(MaterialsTest, ClearingNoShadowsFlagOfTranslucentMaterials)
{
    auto material = GlobalMaterialManager().getMaterial("textures/parsertest/coverage2");

    EXPECT_TRUE(material) << "Could not find the material textures/parsertest/coverage2";
    EXPECT_EQ(material->getCoverage(), Material::MC_TRANSLUCENT) << "Material should be translucent";
    EXPECT_EQ(material->surfaceCastsShadow(), false) << "Material should not cast shadows";
    EXPECT_TRUE(material->getMaterialFlags() & Material::FLAG_NOSHADOWS) << "Material should have the noshadows flag set";

    // It's not possible to clear the noshadows flag as long as translucent is enabled
    material->clearMaterialFlag(Material::FLAG_NOSHADOWS);
    EXPECT_TRUE(material->getMaterialFlags() & Material::FLAG_NOSHADOWS) << "Material should still have noshadows set";
    EXPECT_TRUE(material->getMaterialFlags() & Material::FLAG_TRANSLUCENT) << "Material should still have translucent set";

    // This sequence is allowed
    material->clearMaterialFlag(Material::FLAG_TRANSLUCENT);
    material->clearMaterialFlag(Material::FLAG_NOSHADOWS);
    EXPECT_FALSE(material->getMaterialFlags() & Material::FLAG_NOSHADOWS) << "Material should not have the noshadows flag set";
    EXPECT_FALSE(material->getMaterialFlags() & Material::FLAG_TRANSLUCENT) << "Material should not have the translucent flag set";
}

TEST_F(MaterialsTest, AddLayerToNewMaterial)
{
    auto material = GlobalMaterialManager().createEmptyMaterial("unittest_material");
    material->addLayer(IShaderLayer::DIFFUSE);

    // Check that the added layer is present
    std::vector<IShaderLayer::Ptr> layers;
    material->foreachLayer([&](const IShaderLayer::Ptr& layer)
    {
        layers.push_back(layer);
        return true;
    });

    EXPECT_EQ(layers.size(), 1);
    EXPECT_EQ(layers.front()->getType(), IShaderLayer::DIFFUSE);
}

// When the material name points to a texture file on disk, a default material
// will be created around it, with a simple diffuse layer pointing to that texture
TEST_F(MaterialsTest, CreateMaterialFromTextureFile)
{
    // Copy one of our textures to a temporary path
    auto originalTexturePath = _context.getTestProjectPath() + "textures/numbers/1.tga";
    auto texturePath = string::replace_all_copy(originalTexturePath, "1.tga", "temporary_texture.tga");

    EXPECT_TRUE(fs::exists(originalTexturePath));
    EXPECT_FALSE(fs::exists(texturePath));
    fs::copy(originalTexturePath, texturePath);
    TemporaryFile tempFile(texturePath); // remove when test is done

    // We request a material that is not really present
    EXPECT_FALSE(GlobalDeclarationManager().findDeclaration(decl::Type::Material, "textures/numbers/temporary_texture"));

    auto material = GlobalMaterialManager().getMaterial("textures/numbers/temporary_texture");
    EXPECT_TRUE(material) << "We should have got a material pointing to that texture on disk";

    // Check the layers of this material, it should contain a diffuse map
    std::vector<IShaderLayer::Ptr> layers;
    material->foreachLayer([&](const IShaderLayer::Ptr& layer)
    {
        layers.push_back(layer);
        return true;
    });

    EXPECT_EQ(layers.size(), 1);
    EXPECT_EQ(layers.at(0)->getType(), IShaderLayer::DIFFUSE);
}

// Assigning the syntax to a previously missing material didn't clear the description before reparsing
TEST_F(MaterialsTest, DescriptionClearedBeforeParsing)
{
    auto material = GlobalMaterialManager().getMaterial("this_material_is_missing");

    // This is what the shader library assigns to missing materials
    EXPECT_EQ(material->getDescription(), "This material is missing and has been auto-generated by DarkRadiant");

    // When the material becomes available (e.g. in the course of a mod change) its syntax block will be assigned
    // through setSyntaxBlock(). The description must be cleared as well.
    auto decl = GlobalDeclarationManager().findDeclaration(decl::Type::Material, material->getName());
    EXPECT_TRUE(decl) << "The decl should have been created by the material manager";

    // Assigning a syntax block should trigger a reparse next time the description is requested
    auto newSyntax = decl->getBlockSyntax();
    newSyntax.contents = "\n\n"; // Just an empty decl block without description
    decl->setBlockSyntax(newSyntax);

    // Requesting the description should deliver an empty string now
    // The block itself doesn't declare a description, but the old one must have been cleared nonetheless
    EXPECT_EQ(material->getDescription(), "") << "Description has not been cleared before reparse";
}

// On template change the material emits a materialChanged signal. Requesting the editor image
// in a subscribed changed handler should immediately see the changed editor image, not the old one
TEST_F(MaterialsTest, EditorImageUpdatedBeforeMaterialChangedSignalEmission)
{
    // This is a missing material that should not have an editor image
    auto material = GlobalMaterialManager().getMaterial("this_material_is_missing");

    auto previousEditorImage = material->getEditorImage();
    EXPECT_TRUE(previousEditorImage) << "Even non-existent materials have an editor image";
    EXPECT_TRUE(material->isEditorImageNoTex()) << "Non-existent materials should have a shader-not-found editor image";

    // When the material becomes available (e.g. in the course of a mod change) its syntax block will be assigned
    // through setSyntaxBlock(). The editor image must update to the new syntax
    auto decl = GlobalDeclarationManager().findDeclaration(decl::Type::Material, material->getName());
    EXPECT_TRUE(decl) << "The decl should have been created by the material manager";

    auto receivedEditorImage = TexturePtr();
    auto receivedEditorImageWasNoTex = false;
    auto handlerFired = false;

    // Subscribe to the material changed handler
    material->sig_materialChanged().connect([&]
    {
        handlerFired = true;
        receivedEditorImage = material->getEditorImage();
        receivedEditorImageWasNoTex = material->isEditorImageNoTex();
    });

    // Assigning a syntax block should trigger a reparse next time the description is requested
    auto newSyntax = decl->getBlockSyntax();
    newSyntax.contents = R"(
    qer_editorimage	textures/numbers/0
)";
    decl->setBlockSyntax(newSyntax);

    // Requesting the editor image in the handler should deliver the updated texture now
    EXPECT_TRUE(handlerFired) << "Handler has not been invoked at all";
    EXPECT_NE(receivedEditorImage, previousEditorImage) << "Editor image should have been changed in the handler";
    EXPECT_FALSE(receivedEditorImageWasNoTex) << "Editor image should not be the shader-not-found texture in the handler";

    EXPECT_NE(material->getEditorImage(), previousEditorImage) << "Editor image should have been changed";
    EXPECT_FALSE(material->isEditorImageNoTex()) << "Editor image should have been updated";
}

}
