#include "MaterialSourceGenerator.h"

#include "ShaderTemplate.h"
#include "string/replace.h"
#include "string/join.h"
#include "materials/ParseLib.h"
#include <fmt/format.h>

namespace shaders
{

void writeStageCondition(std::ostream& stream, Doom3ShaderLayer& layer)
{
    // Condition goes first
    if (layer.getConditionExpression())
    {
        stream << "\t\tif " << layer.getConditionExpression()->getExpressionString() << "\n";
    }
}

void writeStageModifiers(std::ostream& stream, Doom3ShaderLayer& layer)
{
    // Alpha Test
    if (layer.hasAlphaTest())
    {
        stream << "\t\talphaTest " << layer.getAlphaTestExpression()->getExpressionString() << "\n";
    }

    // Texture Filter
    if (layer.getStageFlags() & IShaderLayer::FLAG_FILTER_NEAREST)
    {
        stream << "\t\tnearest\n";
    }

    if (layer.getStageFlags() & IShaderLayer::FLAG_FILTER_LINEAR)
    {
        stream << "\t\tlinear\n";
    }

    // Texture Quality
    if (layer.getStageFlags() & IShaderLayer::FLAG_HIGHQUALITY)
    {
        stream << "\t\thighQuality\n";
    }

    if (layer.getStageFlags() & IShaderLayer::FLAG_FORCE_HIGHQUALITY)
    {
        stream << "\t\tforceHighQuality\n";
    }

    if (layer.getStageFlags() & IShaderLayer::FLAG_NO_PICMIP)
    {
        stream << "\t\tnopicmip\n";
    }

    // Texgen
    if (layer.getTexGenType() == IShaderLayer::TEXGEN_REFLECT)
    {
        stream << "\t\ttexgen reflect\n";
    }
    else if (layer.getTexGenType() == IShaderLayer::TEXGEN_SKYBOX)
    {
        stream << "\t\ttexgen skybox\n";
    }
    else if (layer.getTexGenType() == IShaderLayer::TEXGEN_WOBBLESKY)
    {
        auto expr0 = layer.getTexGenExpression(0);
        auto expr1 = layer.getTexGenExpression(1);
        auto expr2 = layer.getTexGenExpression(2);

        stream << "\t\ttexgen wobblesky " <<
            (expr0 ? expr0->getExpressionString() : "") << " " <<
            (expr1 ? expr1->getExpressionString() : "") << " " <<
            (expr2 ? expr2->getExpressionString() : "") << "\n";
    }

    // Clamp Type
    if (layer.getClampType() != CLAMP_REPEAT || layer.hasOverridingClampType())
    {
        stream << "\t\t" << shaders::getStringForClampType(layer.getClampType()) << "\n";
    }

    if (layer.getStageFlags() & IShaderLayer::FLAG_IGNORE_ALPHATEST)
    {
        stream << "\t\tignoreAlphaTest\n";
    }

    if (layer.getStageFlags() & IShaderLayer::FLAG_IGNORE_DEPTH)
    {
        stream << "\t\tignoreDepth\n";
    }

    auto coloredMask = IShaderLayer::FLAG_MASK_RED | IShaderLayer::FLAG_MASK_GREEN | IShaderLayer::FLAG_MASK_BLUE;

    // Summarise red+green+blue to maskColor if possible
    if ((layer.getStageFlags() & coloredMask) == coloredMask)
    {
        stream << "\t\tmaskColor\n";
    }
    else
    {
        if (layer.getStageFlags() & IShaderLayer::FLAG_MASK_RED)
        {
            stream << "\t\tmaskRed\n";
        }

        if (layer.getStageFlags() & IShaderLayer::FLAG_MASK_GREEN)
        {
            stream << "\t\tmaskGreen\n";
        }

        if (layer.getStageFlags() & IShaderLayer::FLAG_MASK_BLUE)
        {
            stream << "\t\tmaskBlue\n";
        }
    }

    if (layer.getStageFlags() & IShaderLayer::FLAG_MASK_ALPHA)
    {
        stream << "\t\tmaskAlpha\n";
    }

    if (layer.getStageFlags() & IShaderLayer::FLAG_MASK_DEPTH)
    {
        stream << "\t\tmaskDepth\n";
    }

    // Vertex colours
    if (layer.getVertexColourMode() == IShaderLayer::VERTEX_COLOUR_MULTIPLY)
    {
        stream << "\t\tvertexColor\n";
    }
    else if (layer.getVertexColourMode() == IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY)
    {
        stream << "\t\tinverseVertexColor\n";
    }

    auto redExpr = layer.getColourExpression(IShaderLayer::COMP_RED);
    auto greenExpr = layer.getColourExpression(IShaderLayer::COMP_GREEN);
    auto blueExpr = layer.getColourExpression(IShaderLayer::COMP_BLUE);
    auto alphaExpr = layer.getColourExpression(IShaderLayer::COMP_ALPHA);

    if (layer.getColourExpression(IShaderLayer::COMP_RGBA))
    {
        // All RGBA components are equivalent
        stream << "\t\trgba " << layer.getColourExpression(IShaderLayer::COMP_RGBA)->getExpressionString() << "\n";
    }
    else if (redExpr && greenExpr && blueExpr && alphaExpr)
    {
        // All 4 expressions in use, check for the colored special case
        if (redExpr->getExpressionString() == "parm0" &&
            greenExpr->getExpressionString() == "parm1" &&
            blueExpr->getExpressionString() == "parm2" &&
            alphaExpr->getExpressionString() == "parm3")
        {
            stream << "\t\tcolored\n";
        }
        // No colored, but RGB+Alpha is still possible
        else if (layer.getColourExpression(IShaderLayer::COMP_RGB))
        {
            stream << "\t\trgb " << layer.getColourExpression(IShaderLayer::COMP_RGB)->getExpressionString() << "\n";
            stream << "\t\talpha " << alphaExpr->getExpressionString() << "\n";
        }
        else // make use of the color shortcut to define all 4 in one line
        {
            stream << "\t\tcolor " << redExpr->getExpressionString() << ", "
                << greenExpr->getExpressionString() << ", "
                << blueExpr->getExpressionString() << ", "
                << alphaExpr->getExpressionString() << "\n";
        }
    }
    else if (layer.getColourExpression(IShaderLayer::COMP_RGB))
    {
        stream << "\t\trgb " << layer.getColourExpression(IShaderLayer::COMP_RGB)->getExpressionString() << "\n";
    }
    else // No shortcuts possible, just write out any non-empty expressions
    {
        if (redExpr)
        {
            stream << "\t\tred " << redExpr->getExpressionString() << "\n";
        }

        if (greenExpr)
        {
            stream << "\t\tgreen " << greenExpr->getExpressionString() << "\n";
        }

        if (blueExpr)
        {
            stream << "\t\tblue " << blueExpr->getExpressionString() << "\n";
        }

        if (alphaExpr)
        {
            stream << "\t\talpha " << alphaExpr->getExpressionString() << "\n";
        }
    }

    // Private Polygon Offset
    if (layer.getPrivatePolygonOffset() != 0)
    {
        stream << "\t\tprivatePolygonOffset " << layer.getPrivatePolygonOffset() << "\n";
    }

    // Stage Transforms
    for (const auto& transform : layer.getTransformations())
    {
        const auto& expr1 = transform.expression1;
        const auto& expr2 = transform.expression2;

        switch (transform.type)
        {
        case IShaderLayer::TransformType::Translate:
            stream << "\t\ttranslate " << (expr1 ? expr1->getExpressionString() : "") << ", " << (expr2 ? expr2->getExpressionString() : "") << "\n";
            break;
        case IShaderLayer::TransformType::Scale:
            stream << "\t\tscale " << (expr1 ? expr1->getExpressionString() : "") << ", " << (expr2 ? expr2->getExpressionString() : "") << "\n";
            break;
        case IShaderLayer::TransformType::CenterScale:
            stream << "\t\tcenterScale " << (expr1 ? expr1->getExpressionString() : "") << ", " << (expr2 ? expr2->getExpressionString() : "") << "\n";
            break;
        case IShaderLayer::TransformType::Shear:
            stream << "\t\tshear " << (expr1 ? expr1->getExpressionString() : "") << ", " << (expr2 ? expr2->getExpressionString() : "") << "\n";
            break;
        case IShaderLayer::TransformType::Rotate:
            stream << "\t\trotate " << (expr1 ? expr1->getExpressionString() : "") << "\n";
            break;
        };
    }

    if (!layer.getVertexProgram().empty() && layer.getVertexProgram() == layer.getFragmentProgram())
    {
        stream << "\t\tprogram " << layer.getVertexProgram() << "\n";
    }
    else if (!layer.getVertexProgram().empty())
    {
        stream << "\t\tvertexProgram " << layer.getVertexProgram() << "\n";
    }
    else if (!layer.getFragmentProgram().empty())
    {
        stream << "\t\tfragmentProgram " << layer.getFragmentProgram() << "\n";
    }

    // Vertex Programs
    if (!layer.getVertexProgram().empty())
    {
        for (int i = 0; i < layer.getNumVertexParms(); ++i)
        {
            const auto& parm = layer.getVertexParm(i);

            if (!parm.expressions[0])
            {
                continue; // skip empty parms
            }

            stream << "\t\tvertexParm " << i << " "
                << (parm.expressions[0] ? parm.expressions[0]->getExpressionString() : "")
                << (parm.expressions[1] ? ", " + parm.expressions[1]->getExpressionString() : "")
                << (parm.expressions[2] ? ", " + parm.expressions[2]->getExpressionString() : "")
                << (parm.expressions[3] ? ", " + parm.expressions[3]->getExpressionString() : "") << "\n";
        }
    }

    // Fragment Maps
    if (!layer.getFragmentProgram().empty())
    {
        for (int i = 0; i < layer.getNumFragmentMaps(); ++i)
        {
            const auto& fragmentMap = layer.getFragmentMap(i);

            if (!fragmentMap.map)
            {
                continue;
            }

            stream << "\t\tfragmentMap " << i << " "
                << (fragmentMap.options.empty() ? "" : string::join(fragmentMap.options, " ") + " ")
                << fragmentMap.map->getExpressionString() << "\n";
        }
    }
}

namespace
{
    // Generates the dimension string " 200 300" including a leading whitespace
    // Generates an empty string if the dimensions are 0,0
    std::string generateRenderMapDimensions(const Doom3ShaderLayer& layer)
    {
        const auto& size = layer.getRenderMapSize();

        return size.getLengthSquared() <= 0 ? "" : fmt::format(" {0} {1}",
            static_cast<int>(layer.getRenderMapSize().x()),
            static_cast<int>(layer.getRenderMapSize().y()));
    }
    
}

void writeBlendMap(std::ostream& stream, const Doom3ShaderLayer& layer)
{
    // Blend types
    const auto& blendFunc = layer.getBlendFuncStrings();

    if (!blendFunc.first.empty() && !isDefaultBlendFunc(blendFunc))
    {
        stream << "\t\tblend " << blendFunc.first;

        if (!blendFunc.second.empty())
        {
            stream << ", " << blendFunc.second << "\n";
        }
        else
        {
            stream << "\n";
        }
    }

    // Map
    auto mapExpr = layer.getMapExpression();

    switch (layer.getMapType())
    {
    case IShaderLayer::MapType::Map:
        stream << "\t\tmap " << (mapExpr ? mapExpr->getExpressionString() : "")  << "\n";
        break;
    case IShaderLayer::MapType::CubeMap:
        stream << "\t\tcubeMap " << (mapExpr ? mapExpr->getExpressionString() : "") << "\n";
        break;
    case IShaderLayer::MapType::CameraCubeMap:
        stream << "\t\tcameraCubeMap " << (mapExpr ? mapExpr->getExpressionString() : "") << "\n";
        break;
    case IShaderLayer::MapType::MirrorRenderMap:
        // Whitespace separator will be generated along with the dimensions
        stream << "\t\tmirrorRenderMap" << generateRenderMapDimensions(layer) << "\n";

        // Mirror render stages are allowed to have map expressions
        if (mapExpr)
        {
            stream << "\t\tmap " << (mapExpr ? mapExpr->getExpressionString() : "") << "\n";
        }

        break;
    case IShaderLayer::MapType::RemoteRenderMap:
        // Whitespace separator will be generated along with the dimensions
        stream << "\t\tremoteRenderMap" << generateRenderMapDimensions(layer) << "\n";

        // Remote render stages are allowed to have map expressions
        if (mapExpr)
        {
            stream << "\t\tmap " << (mapExpr ? mapExpr->getExpressionString() : "") << "\n";
        }

        break;
    case IShaderLayer::MapType::VideoMap:
    {
        auto videoMap = std::dynamic_pointer_cast<IVideoMapExpression>(mapExpr);

        if (videoMap)
        {
            stream << "\t\tvideoMap " << (videoMap->isLooping() ? "loop " : "") << videoMap->getExpressionString() << "\n";
        }
        break;
    }
    case IShaderLayer::MapType::SoundMap:
    {
        auto soundMap = std::dynamic_pointer_cast<ISoundMapExpression>(mapExpr);

        if (soundMap)
        {
            stream << "\t\tsoundMap " << (soundMap->isWaveform() ? "waveform\n" : "\n");
        }
        break;
    }
    } // switch
}

bool stageQualifiesForShortcut(Doom3ShaderLayer& layer)
{
    if (layer.getConditionExpression())
    {
        return false;
    }

    auto mapExpr = layer.getMapExpression();

    if (!mapExpr)
    {
        return false; // no map expression => disqualified
    }

    // Only DBS qualify for shortcuts
    if (layer.getType() != IShaderLayer::DIFFUSE &&
        layer.getType() != IShaderLayer::BUMP &&
        layer.getType() != IShaderLayer::SPECULAR)
    {
        return false;
    }

    // Check the map type, it must be a regular "map"
    return layer.getMapType() == IShaderLayer::MapType::Map;
}

void writeBlendShortcut(std::ostream& stream, Doom3ShaderLayer& layer)
{
    assert(!layer.getConditionExpression());

    auto mapExpr = layer.getMapExpression();
    assert(mapExpr); // this has already been checked by "stageQualifiesForShortcut"

    switch (layer.getType())
    {
    case IShaderLayer::DIFFUSE:  stream << "\tdiffusemap " << mapExpr->getExpressionString() << "\n"; break;
    case IShaderLayer::BUMP:     stream << "\tbumpmap " << mapExpr->getExpressionString() << "\n"; break;
    case IShaderLayer::SPECULAR: stream << "\tspecularmap " << mapExpr->getExpressionString() << "\n"; break;
    default:
        throw std::logic_error("Wrong stage type stranded in writeBlendShortcut");
    };
}

// Write a single layer to the given stream, including curly braces (contents indented by two tabs)
std::ostream& operator<<(std::ostream& stream, Doom3ShaderLayer& layer)
{
    // We're writing all the options to a separate buffer first
    // if the buffer turns out to be empty we can safely switch to the simpler stage shortcuts like "diffusemap _white"
    std::stringstream stageModifierStream;
    writeStageModifiers(stageModifierStream, layer);

    // If we didn't get any modifiers exported, check if the stage has a simple enough image mapping
    if (stageModifierStream.tellp() == 0 && stageQualifiesForShortcut(layer))
    {
        writeBlendShortcut(stream, layer);
    }
    else // Stage is too complex, write the proper block
    {
        stream << "\t{\n";
        writeStageCondition(stream, layer);
        writeBlendMap(stream, layer);
        stream << stageModifierStream.str();
        stream << "\t}\n";
    }

    return stream;
}

// Write the material to the given stream (one tab indentation)
std::ostream& operator<<(std::ostream& stream, ShaderTemplate& shaderTemplate)
{
    stream << "\n";

    if (shaderTemplate.getEditorTexture())
    {
        stream << "\tqer_editorimage " << shaderTemplate.getEditorTexture()->getExpressionString() << "\n";
    }

    if (shaderTemplate.getSurfaceType() != Material::SURFTYPE_DEFAULT)
    {
        stream << "\t" << getStringForSurfaceType(shaderTemplate.getSurfaceType()) << "\n";
    }

    if (!shaderTemplate.getDescription().empty())
    {
        stream << "\tdescription \"" << string::replace_all_copy(shaderTemplate.getDescription(), "\"", "'") << "\"\n";
        stream << "\n";
    }

    // Macros go first
    bool hasDecalMacro = shaderTemplate.getParseFlags() & Material::PF_HasDecalMacro;

    if (hasDecalMacro)
    {
        stream << "\t" << "DECAL_MACRO" << "\n";
    }

    // Go through the material flags which reflect a single keyword
    for (const auto& pair : shaders::MaterialFlagKeywords)
    {
        // Skip exporting noShadows is DECAL_MACRO is active
        if (hasDecalMacro && pair.second == Material::FLAG_NOSHADOWS) continue;

        if (shaderTemplate.getMaterialFlags() & pair.second)
        {
            stream << "\t" << pair.first << "\n";
        }
    }

    // Polygon Offset
    // DECAL_MACRO implies polygonOffset 1, prevent writing redundant information
    if ((shaderTemplate.getMaterialFlags() & Material::FLAG_POLYGONOFFSET) != 0 &&
        (shaderTemplate.getPolygonOffset() != 1 || !hasDecalMacro))
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
    if (shaderTemplate.getMaterialFlags() & Material::FLAG_HAS_SORT_DEFINED && 
        (!hasDecalMacro || shaderTemplate.getSortRequest() != Material::SORT_DECAL))
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

    // DecalInfo
    if (shaderTemplate.getParseFlags() & Material::PF_HasDecalInfo)
    {
        const auto& decalInfo = shaderTemplate.getDecalInfo();
        stream << "\tdecalinfo " << (decalInfo.stayMilliSeconds / 1000.0f) << " "
            << (decalInfo.fadeMilliSeconds / 1000.0f) << " "
            << "( " << decalInfo.startColour.x() << " " << decalInfo.startColour.y() << " " 
            << decalInfo.startColour.z() << " " << decalInfo.startColour.w() << " ) "
            << "( " << decalInfo.endColour.x() << " " << decalInfo.endColour.y() << " "
            << decalInfo.endColour.z() << " " << decalInfo.endColour.w() << " )\n";
    }

    // Renderbump
    if (!shaderTemplate.getRenderBumpArguments().empty())
    {
        stream << "\trenderbump " << shaderTemplate.getRenderBumpArguments() << "\n";
    }

    // Renderbumpflat
    if (!shaderTemplate.getRenderBumpFlatArguments().empty())
    {
        stream << "\trenderbumpflat " << shaderTemplate.getRenderBumpFlatArguments() << "\n";
    }

    // Light Flags
    if (shaderTemplate.isAmbientLight() && shaderTemplate.isCubicLight())
    {
        stream << "\tambientCubicLight\n";
    }
    else
    {
        if (shaderTemplate.isAmbientLight())
        {
            stream << "\tambientLight\n";
        }
        else if (shaderTemplate.isCubicLight())
        {
            stream << "\tcubicLight\n";
        }
    }

    if (shaderTemplate.isFogLight())
    {
        stream << "\tfogLight\n";
    }
    else if (shaderTemplate.isBlendLight())
    {
        stream << "\tblendLight\n";
    }

    if (shaderTemplate.getLightFalloff())
    {
        if (shaderTemplate.getLightFalloffCubeMapType() == IShaderLayer::MapType::CameraCubeMap)
        {
            stream << "\tlightFalloffCubeMap " << shaderTemplate.getLightFalloff()->getExpressionString() << "\n";
        }
        else
        {
            stream << "\tlightFalloffImage " << shaderTemplate.getLightFalloff()->getExpressionString() << "\n";
        }
    }

    // Surface flags
    for (const auto& pair : SurfaceFlags)
    {
        // Skip exporting "discrete" is DECAL_MACRO is active
        if (hasDecalMacro && pair.second == Material::SURF_DISCRETE) continue;

        if (shaderTemplate.getSurfaceFlags() & pair.second)
        {
            stream << "\t" << pair.first << "\n";
        }
    }

    // AmbientRimColor
    if (shaderTemplate.getAmbientRimColourExpression(0) && 
        shaderTemplate.getAmbientRimColourExpression(1) && 
        shaderTemplate.getAmbientRimColourExpression(2))
    {
        stream << "\tambientRimColor " << shaderTemplate.getAmbientRimColourExpression(0)->getExpressionString() << ", " 
            << shaderTemplate.getAmbientRimColourExpression(1)->getExpressionString() << ", "
            << shaderTemplate.getAmbientRimColourExpression(2)->getExpressionString() << "\n";
    }

    for (const auto& layer : shaderTemplate.getLayers())
    {
        stream << *layer;
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
