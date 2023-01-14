#pragma once

#include "i18n.h"
#include "ishaders.h"
#include "ishaderlayer.h"
#include "string/predicate.h"

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

constexpr const char* const FrobCondition = "(parm11 > 0)";
constexpr const char* const AdditiveRgbExpression = "0.15 * parm11";
constexpr const char* const WhiteBlendRgbExpression = "0.4 * parm11";
constexpr const char* const WhiteBlendFuncSrc = "gl_dst_color";
constexpr const char* const WhiteBlendFuncDest = "gl_one";
constexpr const char* const WhiteBlendMap = "_white";

class FrobStageSetup
{
public:
    static bool HasWhiteBlendStage(const MaterialPtr& material)
    {
        return FindWhiteBlendStage(material).first != nullptr;
    }

    static bool HasAdditiveDiffuseStage(const MaterialPtr& material)
    {
        return FindAdditiveDiffuseStage(material).first != nullptr;
    }

    // Checks whether the default frob stages are present on this material
    static bool IsPresent(const MaterialPtr& material)
    {
        if (!material) return false;

        auto diffuse = GetDiffuseMap(material);

        return !diffuse.empty() && HasWhiteBlendStage(material) && HasAdditiveDiffuseStage(material);
    }

    // Add the needed frob stages to this material
    static void AddToMaterial(const MaterialPtr& material)
    {
        if (!material) return;

        if (!HasAdditiveDiffuseStage(material))
        {
            AddAdditiveDiffuseStage(material);
        }

        if (!HasWhiteBlendStage(material))
        {
            AddWhiteBlendStage(material);
        }
    }

    // Removes the two frob stages from this material
    static void RemoveFromMaterial(const MaterialPtr& material)
    {
        if (!material) return;

        auto additiveStage = FindAdditiveDiffuseStage(material);

        if (additiveStage.second > 0)
        {
            material->removeLayer(static_cast<std::size_t>(additiveStage.second));
        }

        auto whiteBlendStage = FindWhiteBlendStage(material);

        if (whiteBlendStage.second > 0)
        {
            material->removeLayer(static_cast<std::size_t>(whiteBlendStage.second));
        }
    }

    static std::string GetDiffuseMap(const MaterialPtr& material)
    {
        auto diffuseMap = std::string();

        material->foreachLayer([&](const IShaderLayer::Ptr& layer)
        {
            if (layer->getType() == IShaderLayer::DIFFUSE && layer->getMapExpression())
            {
                diffuseMap = layer->getMapExpression()->getExpressionString();
                return false;
            }

            return true;
        });

        return diffuseMap;
    }

private:
    static std::vector<IShaderLayer::Ptr> getAllLayers(const MaterialPtr& material)
    {
        std::vector<IShaderLayer::Ptr> layers;
        
        material->foreachLayer([&](const IShaderLayer::Ptr& layer)
        {
            layers.push_back(layer);
            return true;
        });

        return layers;
    }

    static std::pair<IShaderLayer::Ptr, int> FindWhiteBlendStage(const MaterialPtr& material)
    {
        auto layers = getAllLayers(material);

        for (int index = 0; index < layers.size(); ++index)
        {
            const auto& layer = layers[index];

            // if ( parm11 > 0 )
            if (!layer->getConditionExpression() || layer->getConditionExpression()->getExpressionString() != FrobCondition)
            {
                continue;
            }

            // blend gl_dst_color, gl_one
            if (layer->getBlendFuncStrings().first != WhiteBlendFuncSrc || layer->getBlendFuncStrings().second != WhiteBlendFuncDest)
            {
                continue;
            }

            // map _white (or _white.tga)
            if (!layer->getMapExpression() || !string::starts_with(layer->getMapExpression()->getExpressionString(), WhiteBlendMap))
            {
                continue;
            }

            // rgb 0.40 * parm11
            auto rgb = layer->getColourExpression(IShaderLayer::COMP_RGB);

            if (rgb && rgb->getExpressionString() == WhiteBlendRgbExpression)
            {
                return std::make_pair(layer, index);
            }
        }

        return std::make_pair(IShaderLayer::Ptr(), -1);
    }

    static std::pair<IShaderLayer::Ptr, int> FindAdditiveDiffuseStage(const MaterialPtr& material)
    {
        auto diffuse = GetDiffuseMap(material);

        if (diffuse.empty()) return std::make_pair(IShaderLayer::Ptr(), -1);

        auto layers = getAllLayers(material);

        for (int index = 0; index < layers.size(); ++index)
        {
            const auto& layer = layers[index];

            // if ( parm11 > 0 )
            if (!layer->getConditionExpression() || layer->getConditionExpression()->getExpressionString() != FrobCondition)
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

            if (rgb && rgb->getExpressionString() == AdditiveRgbExpression)
            {
                return std::make_pair(layer, index);
            }
        }

        return std::make_pair(IShaderLayer::Ptr(), -1);
    }

    static void AddAdditiveDiffuseStage(const MaterialPtr& material)
    {
        auto index = material->addLayer(IShaderLayer::BLEND);
        auto stage = material->getEditableLayer(index);

        auto diffuse = GetDiffuseMap(material);

        if (diffuse.empty())
        {
            throw std::runtime_error(_("No diffusemap present, cannot add the frob stages."));
        }

        stage->setConditionExpressionFromString(FrobCondition);
        stage->setBlendFuncStrings({ "add", "" });
        stage->setMapExpressionFromString(diffuse);
        stage->setColourExpressionFromString(IShaderLayer::COMP_RGB, AdditiveRgbExpression);
    }

    static void AddWhiteBlendStage(const MaterialPtr& material)
    {
        auto index = material->addLayer(IShaderLayer::BLEND);
        auto stage = material->getEditableLayer(index);

        stage->setConditionExpressionFromString(FrobCondition);
        stage->setBlendFuncStrings({ WhiteBlendFuncSrc, WhiteBlendFuncDest });
        stage->setMapExpressionFromString(WhiteBlendMap);
        stage->setColourExpressionFromString(IShaderLayer::COMP_RGB, WhiteBlendRgbExpression);
    }
};

}
