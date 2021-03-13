#pragma once

#include <map>
#include "ishaders.h"
#include "ishaderlayer.h"

namespace shaders
{

constexpr std::pair<const char*, Material::SurfaceType> SurfaceTypeMapping[]
{
    { "metal", Material::SURFTYPE_METAL },
    { "stone", Material::SURFTYPE_STONE },
    { "flesh", Material::SURFTYPE_FLESH },
    { "wood", Material::SURFTYPE_WOOD },
    { "cardboard", Material::SURFTYPE_CARDBOARD },
    { "liquid", Material::SURFTYPE_LIQUID },
    { "glass", Material::SURFTYPE_GLASS },
    { "plastic", Material::SURFTYPE_PLASTIC },
    { "ricochet", Material::SURFTYPE_RICOCHET },
    { "surftype10", Material::SURFTYPE_10 },
    { "surftype11", Material::SURFTYPE_11 },
    { "surftype12", Material::SURFTYPE_12 },
    { "surftype13", Material::SURFTYPE_13 },
    { "surftype14", Material::SURFTYPE_14 },
    { "surftype15", Material::SURFTYPE_15 }
};

inline std::string getStringForSurfaceType(Material::SurfaceType type)
{
    for (const auto& pair : SurfaceTypeMapping)
    {
        if (type == pair.second)
        {
            return pair.first;
        }
    }

    return std::string();
}

inline Material::SurfaceType getSurfaceTypeForString(const std::string& surfaceTypeString)
{
    for (const auto& pair : SurfaceTypeMapping)
    {
        if (surfaceTypeString == pair.first)
        {
            return pair.second;
        }
    }

    return Material::SURFTYPE_DEFAULT;
}

constexpr std::pair<const char*, Material::SortRequest> PredefinedSortValues[]
{
    { "subview", Material::SORT_SUBVIEW },
    { "opaque", Material::SORT_OPAQUE },
    { "decal", Material::SORT_DECAL },
    { "far", Material::SORT_FAR },
    { "medium", Material::SORT_MEDIUM },
    { "close", Material::SORT_CLOSE },
    { "almostnearest", Material::SORT_ALMOST_NEAREST },
    { "nearest", Material::SORT_NEAREST },
    { "afterfog", Material::SORT_AFTER_FOG },
    { "postprocess", Material::SORT_POST_PROCESS },
    { "portalsky", Material::SORT_PORTAL_SKY },
};

inline std::string getStringForSortRequestValue(float value)
{
    for (const auto& pair : PredefinedSortValues)
    {
        if (value == pair.second)
        {
            return pair.first;
        }
    }

    return std::string();
}

constexpr std::pair<const char*, Material::DeformType> DeformTypeNames[]
{
    { "sprite", Material::DEFORM_SPRITE },
    { "tube", Material::DEFORM_TUBE },
    { "flare", Material::DEFORM_FLARE },
    { "expand", Material::DEFORM_EXPAND },
    { "move", Material::DEFORM_MOVE },
    { "turbulent", Material::DEFORM_TURBULENT },
    { "eyeball", Material::DEFORM_EYEBALL },
    { "particle", Material::DEFORM_PARTICLE },
    { "particle2", Material::DEFORM_PARTICLE2 },
};

inline std::string getStringForDeformType(Material::DeformType type)
{
    for (const auto& pair : DeformTypeNames)
    {
        if (type == pair.second)
        {
            return pair.first;
        }
    }

    return std::string();
}

constexpr std::pair<const char*, std::pair<const char*, const char*>> BlendTypeShortcuts[]
{
    { "blend", { "gl_src_alpha", "gl_one_minus_src_alpha" } },
    { "add", { "gl_one", "gl_one" } },
    { "filter", { "gl_dst_color", "gl_zero" } },
    { "modulate", { "gl_dst_color", "gl_zero" } },
    { "none", { "gl_zero", "gl_one" } },
};

constexpr std::pair<const char*, ShaderLayer::TexGenType> TexGenTypeNames[]
{
    { "normal", ShaderLayer::TEXGEN_NORMAL },
    { "reflect", ShaderLayer::TEXGEN_REFLECT },
    { "skybox", ShaderLayer::TEXGEN_SKYBOX },
    { "wobbleSky", ShaderLayer::TEXGEN_WOBBLESKY },
};

inline std::string getStringForTexGenType(ShaderLayer::TexGenType type)
{
    for (const auto& pair : TexGenTypeNames)
    {
        if (type == pair.second)
        {
            return pair.first;
        }
    }

    return std::string();
}

constexpr int NUM_MAX_VERTEX_PARMS = 4;
constexpr int NUM_MAX_FRAGMENT_MAPS = 8;

}
