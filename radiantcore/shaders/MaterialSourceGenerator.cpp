#include "MaterialSourceGenerator.h"

#include "ShaderTemplate.h"
#include "string/replace.h"
#include "materials/ParseLib.h"
#include <fmt/format.h>

namespace shaders
{

std::ostream& operator<<(std::ostream& stream, ShaderTemplate& shaderTemplate)
{
    stream << "\n";

    if (shaderTemplate.getSurfaceType() != Material::SURFTYPE_DEFAULT)
    {
        stream << "\t" << getStringForSurfaceType(shaderTemplate.getSurfaceType()) << "\n";
    }

    if (!shaderTemplate.getDescription().empty())
    {
        stream << "\tdescription \"" << string::replace_all_copy(shaderTemplate.getDescription(), "\"", "'") << "\"\n";
    }

    // Go through the material flags which reflect a single keyword
    for (const auto& pair : shaders::MaterialFlagKeywords)
    {
        if (shaderTemplate.getMaterialFlags() & pair.second)
        {
            stream << "\t" << pair.first << "\n";
        }
    }

    if (shaderTemplate.getMaterialFlags() & Material::FLAG_POLYGONOFFSET)
    {
        stream << fmt::format("\tpolygonOffset {0}\n", shaderTemplate.getPolygonOffset());
    }

    if (shaderTemplate.getClampType() != CLAMP_REPEAT)
    {
        stream << "\t" << getStringForClampType(shaderTemplate.getClampType()) << "\n";
    }

    for (const auto& layer : shaderTemplate.getLayers())
    {
        stream << "\t{\n";

        stream << "\t}\n";
    }

    return stream;
}

std::string MaterialSourceGenerator::GenerateDefinitionBlock(ShaderTemplate& shaderTemplate)
{
    std::stringstream output;

    output << shaderTemplate;

    return output.str();
}

}
