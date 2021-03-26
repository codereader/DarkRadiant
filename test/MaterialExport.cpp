#include "RadiantTest.h"

#include "ishaders.h"
#include "string/trim.h"
#include "materials/ParseLib.h"
#include <fmt/format.h>

namespace test
{

using MaterialExportTest = RadiantTest;

inline void expectDefinitionContains(const MaterialPtr& material, const std::string& expectedContainedString)
{
    EXPECT_NE(material->getDefinition().find(expectedContainedString), std::string::npos) 
        << "Material definition doesn't contain " << expectedContainedString << " as expected.\n" 
        << "Definition was: \n" << material->getDefinition();
}

inline void expectDefinitionDoesNotContain(const MaterialPtr& material, const std::string& expectedContainedString)
{
    EXPECT_EQ(material->getDefinition().find(expectedContainedString), std::string::npos)
        << "Material definition contains " << expectedContainedString << " but that shouldn't be the case.\n"
        << "Definition was: \n" << material->getDefinition();
}

TEST_F(MaterialExportTest, Description)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    constexpr auto description = "testdescription, with commas, spaces and extra";
    material->setDescription(description);

    expectDefinitionContains(material, fmt::format("description \"{0}\"", description));

    constexpr auto doubleQuoted = "testdescription with \"quotes\"";
    constexpr auto singleQuoted = "testdescription with 'quotes'";
    material->setDescription(doubleQuoted);

    expectDefinitionContains(material, fmt::format("description \"{0}\"", singleQuoted));
}

TEST_F(MaterialExportTest, PolygonOffset)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    material->setPolygonOffset(0.0);
    expectDefinitionContains(material, fmt::format("polygonOffset {0}", 0.0));

    material->setPolygonOffset(-1.5);
    expectDefinitionContains(material, fmt::format("polygonOffset {0}", -1.5));

    material->setPolygonOffset(+1.5);
    expectDefinitionContains(material, fmt::format("polygonOffset {0}", 1.5));

    material->clearMaterialFlag(Material::FLAG_POLYGONOFFSET);
    expectDefinitionDoesNotContain(material, "polygonOffset");
}

TEST_F(MaterialExportTest, SurfaceType)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    for (const auto& pair : shaders::SurfaceTypeMapping)
    {
        material->setSurfaceType(pair.second);
        expectDefinitionContains(material, pair.first);
    }

    // Test resetting the surface type to default which should clear the type
    auto lastSurfaceType = shaders::getStringForSurfaceType(material->getSurfaceType());
    EXPECT_NE(lastSurfaceType, std::string());

    material->setSurfaceType(Material::SURFTYPE_DEFAULT);
    expectDefinitionDoesNotContain(material, "lastSurfaceType");
}

TEST_F(MaterialExportTest, MaterialFlags)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    for (const auto& pair : shaders::MaterialFlagKeywords)
    {
        material->setMaterialFlag(pair.second);
        expectDefinitionContains(material, pair.first);

        material->clearMaterialFlag(pair.second);
        expectDefinitionDoesNotContain(material, pair.first);
    }
}

TEST_F(MaterialExportTest, ClampType)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    material->setClampType(CLAMP_NOREPEAT);
    expectDefinitionContains(material, "clamp");

    material->setClampType(CLAMP_ZEROCLAMP);
    expectDefinitionContains(material, "zeroclamp");

    material->setClampType(CLAMP_ALPHAZEROCLAMP);
    expectDefinitionContains(material, "alphazeroclamp");

    material->setClampType(CLAMP_REPEAT); // this is the default => no keyword necessary
    expectDefinitionDoesNotContain(material, "clamp");
    expectDefinitionDoesNotContain(material, "zeroclamp");
    expectDefinitionDoesNotContain(material, "alphazeroclamp");
}

TEST_F(MaterialExportTest, CullType)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    material->setCullType(Material::CULL_FRONT);
    expectDefinitionContains(material, "backsided");

    material->setCullType(Material::CULL_NONE);
    expectDefinitionContains(material, "twosided");

    material->setCullType(Material::CULL_BACK); // This is the default
    expectDefinitionDoesNotContain(material, "twosided");
    expectDefinitionDoesNotContain(material, "backsided");
}

TEST_F(MaterialExportTest, GuiSurf)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/guisurf1");
    expectDefinitionContains(material, "guiSurf\tguis/lvlmaps/genericmap.gui");

    // Mark the definition as modified by setting the description
    material->setDescription("-");
    material->setDescription("");

    expectDefinitionContains(material, "guisurf guis/lvlmaps/genericmap.gui");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/guisurf2");
    expectDefinitionContains(material, "guiSurf\tentity");

    // Mark the definition as modified by setting the description
    material->setDescription("-");
    material->setDescription("");

    expectDefinitionContains(material, "guisurf entity");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/guisurf3");
    expectDefinitionContains(material, "guiSurf\tentity2");

    // Mark the definition as modified by setting the description
    material->setDescription("-");
    material->setDescription("");

    expectDefinitionContains(material, "guisurf entity2");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/guisurf4");
    expectDefinitionContains(material, "guiSurf\tentity3");

    // Mark the definition as modified by setting the description
    material->setDescription("-");
    material->setDescription("");

    expectDefinitionContains(material, "guisurf entity3");
}

TEST_F(MaterialExportTest, Sort)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    material->setSortRequest(-1.2f);
    expectDefinitionContains(material, "sort -1.2");

    // Using pre-defined sort values should produce the corresponding string, like "subview"
    for (const auto& pair : shaders::PredefinedSortValues)
    {
        material->setSortRequest(static_cast<float>(pair.second));
        expectDefinitionContains(material, fmt::format("sort {0}", pair.first));
    }

    material->clearMaterialFlag(Material::FLAG_HAS_SORT_DEFINED);
    expectDefinitionDoesNotContain(material, "sort");
}

TEST_F(MaterialExportTest, Spectrum)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    for (int i = -50; i < 50; ++i)
    {
        material->setSpectrum(i);

        if (i != 0)
        {
            expectDefinitionContains(material, fmt::format("spectrum {0}", i));
        }
        else // spectrum 0 is the default, doesn't need to be declared
        {
            expectDefinitionDoesNotContain(material, "spectrum");
        }
    }
}

TEST_F(MaterialExportTest, Deform)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/deform1");
    expectDefinitionContains(material, "deform");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "deform flare 1.5");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/deform2");
    expectDefinitionContains(material, "deform");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "deform expand (0.1 * deformexporttesttable[time * (0.3 + time)] - global3)");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/deform3");
    expectDefinitionContains(material, "deform");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "deform move (1.7 + time + 4.0 - global3)");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/deform4");
    expectDefinitionContains(material, "deform");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "deform turbulent deformexporttesttable time * 2.0 (parm11 - 4.0) -1.0 * global5");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/deform5");
    expectDefinitionContains(material, "deform");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "deform particle testparticle");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/deform6");
    expectDefinitionContains(material, "deform");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "deform particle2 testparticle");
}

TEST_F(MaterialExportTest, DecalInfo)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/decalinfo");
    expectDefinitionContains(material, "decalinfo");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "decalinfo 14.3 1.5 ( 0.9 0.8 0.7 0.6 ) ( 0.5 0.5 0.4 0.3 )");
}

TEST_F(MaterialExportTest, RenderBump)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/renderBump1");
    expectDefinitionContains(material, "renderBump");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "renderbump textures/output.tga models/hipoly");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/renderBump2");
    expectDefinitionContains(material, "renderBump");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "renderbump -size 100 200 textures/output.tga models/hipoly");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/renderBump3");
    expectDefinitionContains(material, "renderBump");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "renderbump -aa 2 textures/output.tga models/hipoly");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/renderBump4");
    expectDefinitionContains(material, "renderBump");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "renderbump -aa 2 -size 10 10 textures/output.tga models/hipoly");
}

TEST_F(MaterialExportTest, RenderBumpFlat)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/renderBumpFlat1");
    expectDefinitionContains(material, "renderBumpflat");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "renderbumpflat -size 200 100 models/hipoly");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/renderBumpFlat2");
    expectDefinitionContains(material, "renderBumpflat");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "renderbumpflat models/hipoly");
}

TEST_F(MaterialExportTest, LightFlags)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    material->setIsAmbientLight(true);
    expectDefinitionContains(material, "ambientLight");
    material->setIsAmbientLight(false);
    expectDefinitionDoesNotContain(material, "ambientLight");

    material->setIsBlendLight(true);
    expectDefinitionContains(material, "blendLight");
    material->setIsBlendLight(false);
    expectDefinitionDoesNotContain(material, "blendLight");

    material->setIsFogLight(true);
    expectDefinitionContains(material, "fogLight");
    material->setIsFogLight(false);
    expectDefinitionDoesNotContain(material, "fogLight");

    material->setIsCubicLight(true);
    expectDefinitionContains(material, "cubicLight");
    material->setIsCubicLight(false);
    expectDefinitionDoesNotContain(material, "cubicLight");

    material->setIsCubicLight(true);
    material->setIsAmbientLight(true);
    expectDefinitionContains(material, "ambientCubicLight");

    material->setIsAmbientLight(false);
    material->setIsCubicLight(false);
    expectDefinitionDoesNotContain(material, "ambientCubicLight");
}

TEST_F(MaterialExportTest, LightFalloffImage)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    material->setLightFalloffCubeMapType(IShaderLayer::MapType::Map);
    material->setLightFalloffExpressionFromString("makeintensity(lights/standard)");
    expectDefinitionContains(material, "lightFalloffImage makeIntensity(lights/standard)");

    material->setLightFalloffExpressionFromString("");
    expectDefinitionDoesNotContain(material, "lightFalloffImage");

    material->setLightFalloffCubeMapType(IShaderLayer::MapType::CameraCubeMap);
    material->setLightFalloffExpressionFromString("env/standard");
    expectDefinitionContains(material, "lightFalloffCubeMap env/standard");
}

}
