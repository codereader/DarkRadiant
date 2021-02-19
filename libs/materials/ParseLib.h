#pragma once

#include <map>
#include "ishaders.h"

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

}
