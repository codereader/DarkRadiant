#pragma once

#include <map>
#include <stdexcept>
#include "ishaders.h"
#include "ishaderlayer.h"
#include "gamelib.h"
#include "os/path.h"
#include "string/case_conv.h"
#include "xmlutil/MissingXMLNodeException.h"

namespace shaders
{

constexpr const char* MISSING_BASEPATH_NODE = "Failed to find \"/game/filesystem/shaders/basepath\" node in game descriptor";
constexpr const char* MISSING_EXTENSION_NODE = "Failed to find \"/game/filesystem/shaders/extension\" node in game descriptor";

// returns the materials folder with a trailing slash, e.g. "materials/"
inline std::string getMaterialsFolderName()
{
    auto nodes = game::current::getNodes("/filesystem/shaders/basepath");

    if (nodes.empty())
    {
        throw xml::MissingXMLNodeException(MISSING_BASEPATH_NODE);
    }

    return os::standardPathWithSlash(nodes[0].getContent());
}

inline std::string getMaterialFileExtension()
{
    auto nodes = game::current::getNodes("/filesystem/shaders/extension");

    if (nodes.empty())
    {
        throw xml::MissingXMLNodeException(MISSING_EXTENSION_NODE);
    }

    return nodes[0].getContent();
}

// A regex pattern that can be used to detect a material name in a single line of a material file
// which is optionally detecting an opening curly brace in the same line
inline std::string getDeclNamePatternForMaterialName(const std::string& name)
{
    return "^\\s*" + name + "\\s*(\\{*).*$";
}

constexpr std::pair<const char*, Material::Flags> MaterialFlagKeywords[]
{
    { "noShadows", Material::FLAG_NOSHADOWS },
    { "noSelfShadow", Material::FLAG_NOSELFSHADOW },
    { "forceShadows", Material::FLAG_FORCESHADOWS },
    { "noOverlays", Material::FLAG_NOOVERLAYS },
    { "forceOverlays", Material::FLAG_FORCEOVERLAYS },
    { "translucent", Material::FLAG_TRANSLUCENT },
    { "forceOpaque", Material::FLAG_FORCEOPAQUE },
    { "noFog", Material::FLAG_NOFOG },
    { "noPortalFog", Material::FLAG_NOPORTALFOG },
    { "unsmoothedTangents", Material::FLAG_UNSMOOTHEDTANGENTS },
    { "mirror", Material::FLAG_MIRROR },
    { "isLightgemSurf", Material::FLAG_ISLIGHTGEMSURF },
};

inline std::string getStringForMaterialFlag(Material::Flags flag)
{
    for (const auto& pair : MaterialFlagKeywords)
    {
        if (flag == pair.second)
        {
            return pair.first;
        }
    }

    return std::string();
}

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

// Surface flags with a single corresponding keyword
constexpr std::pair<const char*, Material::SurfaceFlags> SurfaceFlags[]
{
    { "solid", Material::SURF_SOLID },
    { "water", Material::SURF_WATER },
    { "playerclip", Material::SURF_PLAYERCLIP },
    { "monsterclip", Material::SURF_MONSTERCLIP },
    { "moveableclip", Material::SURF_MOVEABLECLIP },
    { "ikclip", Material::SURF_IKCLIP },
    { "blood", Material::SURF_BLOOD },
    { "trigger", Material::SURF_TRIGGER },
    { "aassolid", Material::SURF_AASSOLID },
    { "aasobstacle", Material::SURF_AASOBSTACLE },
    { "flashlight_trigger", Material::SURF_FLASHLIGHT_TRIGGER },
    { "nonsolid", Material::SURF_NONSOLID },
    { "nullnormal", Material::SURF_NULLNORMAL },
    { "areaportal", Material::SURF_AREAPORTAL },
    { "qer_nocarve", Material::SURF_NOCARVE },
    { "discrete", Material::SURF_DISCRETE },
    { "nofragment", Material::SURF_NOFRAGMENT },
    { "slick", Material::SURF_SLICK },
    { "collision", Material::SURF_COLLISION },
    { "noimpact", Material::SURF_NOIMPACT },
    { "nodamage", Material::SURF_NODAMAGE },
    { "ladder", Material::SURF_LADDER },
    { "nosteps", Material::SURF_NOSTEPS },
};

constexpr std::pair<const char*, Material::CullType> CullTypes[]
{
    { "frontsided", Material::CULL_BACK },
    { "backsided", Material::CULL_FRONT },
    { "twosided", Material::CULL_NONE },
};

inline std::string getStringForCullType(Material::CullType type)
{
    for (const auto& pair : CullTypes)
    {
        if (type == pair.second)
        {
            return pair.first;
        }
    }

    return CullTypes[0].first;
}

inline Material::CullType getCullTypeForString(const std::string& typeString)
{
    for (const auto& pair : CullTypes)
    {
        if (typeString == pair.first)
        {
            return pair.second;
        }
    }

    return Material::CULL_BACK;
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

inline float getSortRequestValueForString(const std::string& value)
{
    for (const auto& pair : PredefinedSortValues)
    {
        if (value == pair.first)
        {
            return static_cast<float>(pair.second);
        }
    }

    return string::convert<float>(value, Material::SORT_OPAQUE);
}

constexpr std::pair<const char*, IShaderLayer::MapType> MapTypeNames[]
{
    { "map", IShaderLayer::MapType::Map },
    { "cubeMap", IShaderLayer::MapType::CubeMap },
    { "cameraCubeMap", IShaderLayer::MapType::CameraCubeMap },
    { "mirrorRenderMap", IShaderLayer::MapType::MirrorRenderMap },
    { "remoteRenderMap", IShaderLayer::MapType::RemoteRenderMap },
    { "videoMap", IShaderLayer::MapType::VideoMap },
    { "soundMap", IShaderLayer::MapType::SoundMap },
};

inline std::string getStringForMapType(IShaderLayer::MapType type)
{
    for (const auto& pair : MapTypeNames)
    {
        if (type == pair.second)
        {
            return pair.first;
        }
    }

    return std::string();
}

inline IShaderLayer::MapType getMapTypeForString(const std::string& typeString)
{
    for (const auto& pair : MapTypeNames)
    {
        if (typeString == pair.first)
        {
            return pair.second;
        }
    }

    return IShaderLayer::MapType::Map;
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

inline bool isDefaultBlendFunc(const std::pair<std::string, std::string>& blendFuncStrings)
{
    return string::to_lower_copy(blendFuncStrings.first) == "gl_one" && 
           string::to_lower_copy(blendFuncStrings.second) == "gl_zero";
}

constexpr std::pair<const char*, IShaderLayer::TexGenType> TexGenTypeNames[]
{
    { "normal", IShaderLayer::TEXGEN_NORMAL },
    { "reflect", IShaderLayer::TEXGEN_REFLECT },
    { "skybox", IShaderLayer::TEXGEN_SKYBOX },
    { "wobbleSky", IShaderLayer::TEXGEN_WOBBLESKY },
};

inline std::string getStringForTexGenType(IShaderLayer::TexGenType type)
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

inline IShaderLayer::TexGenType getTexGenTypeForString(const std::string& typeString)
{
    for (const auto& pair : TexGenTypeNames)
    {
        if (typeString == pair.first)
        {
            return pair.second;
        }
    }

    return IShaderLayer::TexGenType::TEXGEN_NORMAL;
}

constexpr std::pair<const char*, IShaderLayer::TransformType> TransformTypeNames[]
{
    { "Translate", IShaderLayer::TransformType::Translate },
    { "Scale", IShaderLayer::TransformType::Scale },
    { "CenterScale", IShaderLayer::TransformType::CenterScale },
    { "Shear", IShaderLayer::TransformType::Shear },
    { "Rotate", IShaderLayer::TransformType::Rotate },
};

inline std::string getStringForTransformType(IShaderLayer::TransformType type)
{
    for (const auto& pair : TransformTypeNames)
    {
        if (type == pair.second)
        {
            return pair.first;
        }
    }

    return std::string();
}

inline IShaderLayer::TransformType getTransformTypeForString(const std::string& typeString)
{
    for (const auto& pair : TransformTypeNames)
    {
        if (typeString == pair.first)
        {
            return pair.second;
        }
    }

    return IShaderLayer::TransformType::Translate;
}

constexpr std::pair<const char*, ClampType> ClampTypeNames[]
{
    { "noclamp", CLAMP_REPEAT },
    { "clamp", CLAMP_NOREPEAT },
    { "zeroclamp", CLAMP_ZEROCLAMP },
    { "alphazeroclamp", CLAMP_ALPHAZEROCLAMP },
};

inline ClampType getClampTypeForString(const std::string& typeString)
{
    for (const auto& pair : ClampTypeNames)
    {
        if (typeString == pair.first)
        {
            return pair.second;
        }
    }

    return CLAMP_REPEAT;
}

inline std::string getStringForClampType(ClampType type)
{
    for (const auto& pair : ClampTypeNames)
    {
        if (type == pair.second)
        {
            return pair.first;
        }
    }

    return std::string();
}

constexpr std::pair<Material::FrobStageType, const char*> FrobStageTypeNames[]
{
    { Material::FrobStageType::Default, "" },
    { Material::FrobStageType::Diffuse, "frobstage_diffuse" },
    { Material::FrobStageType::Texture, "frobstage_texture" },
    { Material::FrobStageType::NoFrobStage, "frobstage_none" },
};

inline std::string getStringForFrobStageType(Material::FrobStageType type)
{
    for (const auto& pair : FrobStageTypeNames)
    {
        if (type == pair.first)
        {
            return pair.second;
        }
    }

    return std::string();
}

constexpr int NUM_MAX_VERTEX_PARMS = 4;
constexpr int NUM_MAX_FRAGMENT_MAPS = 8;

}
