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
        stream << "\n";
    }

    // Go through the material flags which reflect a single keyword
    for (const auto& pair : shaders::MaterialFlagKeywords)
    {
        if (shaderTemplate.getMaterialFlags() & pair.second)
        {
            stream << "\t" << pair.first << "\n";
        }
    }

    // Polygon Offset
    if (shaderTemplate.getMaterialFlags() & Material::FLAG_POLYGONOFFSET)
    {
        stream << fmt::format("\tpolygonOffset {0}\n", shaderTemplate.getPolygonOffset());
    }

    // Clamping
    if (shaderTemplate.getClampType() != CLAMP_REPEAT)
    {
        stream << "\t" << getStringForClampType(shaderTemplate.getClampType()) << "\n";
    }

    // Culling
    if (shaderTemplate.getCullType() != Material::CULL_BACK)
    {
        stream << "\t" << getStringForCullType(shaderTemplate.getCullType()) << "\n";
    }

    // GuiSurf
    if (shaderTemplate.getSurfaceFlags() & Material::SURF_GUISURF)
    {
        stream << "\tguisurf ";

        if (shaderTemplate.getSurfaceFlags() & Material::SURF_ENTITYGUI)
        {
            stream << "entity\n";
        }
        else if (shaderTemplate.getSurfaceFlags() & Material::SURF_ENTITYGUI2)
        {
            stream << "entity2\n";
        }
        else if (shaderTemplate.getSurfaceFlags() & Material::SURF_ENTITYGUI3)
        {
            stream << "entity3\n";
        }
        else
        {
            stream << shaderTemplate.getGuiSurfArgument() << "\n";
        }
    }

    // Sort
    if (shaderTemplate.getMaterialFlags() & Material::FLAG_HAS_SORT_DEFINED)
    {
        stream << "\tsort ";

        auto predefinedName = getStringForSortRequestValue(shaderTemplate.getSortRequest());

        if (!predefinedName.empty())
        {
            stream << predefinedName << "\n";
        }
        else
        {
            stream << shaderTemplate.getSortRequest() << "\n";
        }
    }

    // Spectrum
    if (shaderTemplate.getSpectrum() != 0)
    {
        stream << "\tspectrum " << shaderTemplate.getSpectrum() << "\n";
    }

    if (shaderTemplate.getDeformType() != Material::DEFORM_NONE)
    {
        stream << "\tdeform " << getStringForDeformType(shaderTemplate.getDeformType());

        switch (shaderTemplate.getDeformType())
        {
        case Material::DEFORM_SPRITE:
        case Material::DEFORM_TUBE:
        case Material::DEFORM_EYEBALL:
            stream << "\n";
            break;
        case Material::DEFORM_FLARE:
        case Material::DEFORM_EXPAND:
        case Material::DEFORM_MOVE:
        {
            auto deformExpression = shaderTemplate.getDeformExpression(0);
            stream << " " << (deformExpression ? deformExpression->getExpressionString() : "") << "\n";
            break;
        }
        case Material::DEFORM_TURBULENT:
        {
            auto deformExpression0 = shaderTemplate.getDeformExpression(0);
            auto deformExpression1 = shaderTemplate.getDeformExpression(1);
            auto deformExpression2 = shaderTemplate.getDeformExpression(2);

            stream << " " << shaderTemplate.getDeformDeclName() << " "
                << (deformExpression0 ? deformExpression0->getExpressionString() : "") << " "
                << (deformExpression1 ? deformExpression1->getExpressionString() : "") << " "
                << (deformExpression2 ? deformExpression2->getExpressionString() : "") << "\n";
            break;
        }
        case Material::DEFORM_PARTICLE:
        case Material::DEFORM_PARTICLE2:
            stream << " " << shaderTemplate.getDeformDeclName() << "\n";
            break;
        }
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
