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

}
