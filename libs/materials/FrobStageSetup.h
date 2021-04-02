#pragma once

#include "ishaders.h"
#include "ishaderlayer.h"

namespace shaders
{

/**
 * Helper class handling the two default frob stages on TDM materials.
 * The two frob stages look like this:
 * 
 * // Blend _white stage
 * {
 *     if ( parm11 > 0 )
 *     blend       gl_dst_color, gl_one
 *     map         _white
 *     rgb         0.40 * parm11
 * }
 * 
 * // Additive blend of the material's diffuse map
 * {
 *     if ( parm11 > 0 )
 *     blend       add
 *     map         textures/numbers/0
 *     rgb         0.15 * parm11
 * }
 */
class FrobStageSetup
{
public:
    static bool HasWhiteBlendStage(const MaterialPtr& material)
    {
        for (const auto& layer : material->getAllLayers())
        {
            // if ( parm11 > 0 )
            if (!layer->getConditionExpression() || layer->getConditionExpression()->getExpressionString() != "(parm11 > 0.0)")
            {
                continue;
            }

            // blend gl_dst_color, gl_one
            if (layer->getBlendFuncStrings().first != "gl_dst_color" || layer->getBlendFuncStrings().second != "gl_one")
            {
                continue;
            }

            // map _white
            if (!layer->getMapExpression() || layer->getMapExpression()->getExpressionString() != "_white")
            {
                continue;
            }

            // rgb 0.40 * parm11
            auto rgb = layer->getColourExpression(IShaderLayer::COMP_RGB);

            if (rgb && rgb->getExpressionString() == "0.4 * parm11")
            {
                return true;
            }
        }

        return false;
    }

    static bool HasAdditiveDiffuseStage(const MaterialPtr& material)
    {
        auto diffuse = GetDiffuseMap(material);

        if (diffuse.empty()) return false;

        for (const auto& layer : material->getAllLayers())
        {
            // if ( parm11 > 0 )
            if (!layer->getConditionExpression() || layer->getConditionExpression()->getExpressionString() != "(parm11 > 0.0)")
            {
                continue;
            }

            // blend add
            if (layer->getBlendFuncStrings().first != "add" || !layer->getBlendFuncStrings().second.empty())
            {
                continue;
            }

            // map <diffuse>
            if (!layer->getMapExpression() || layer->getMapExpression()->getExpressionString() != diffuse)
            {
                continue;
            }

            // rgb 0.15 * parm11
            auto rgb = layer->getColourExpression(IShaderLayer::COMP_RGB);

            if (rgb && rgb->getExpressionString() == "0.15 * parm11")
            {
                return true;
            }
        }

        return false;
    }

    // Checks whether the default frob stages are present on this material
    static bool IsPresent(const MaterialPtr& material)
    {
        if (!material) return false;

        auto diffuse = GetDiffuseMap(material);

        return !diffuse.empty() && HasWhiteBlendStage(material) && HasAdditiveDiffuseStage(material);
    }

private:
    static std::string GetDiffuseMap(const MaterialPtr& material)
    {
        for (const auto& layer : material->getAllLayers())
        {
            if (layer->getType() == IShaderLayer::DIFFUSE && layer->getMapExpression())
            {
                return layer->getMapExpression()->getExpressionString();
            }
        }

        return std::string();
    }
};

}
