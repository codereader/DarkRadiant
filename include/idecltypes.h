#pragma once

#include <stdexcept>
#include <string>
#include "i18n.h"

namespace decl
{

// Enumeration of declaration types supported by DarkRadiant
enum class Type
{
    None,
    Material,
    EntityDef,
    SoundShader,
    Model,
    Particle,
    Skin,
};

inline std::string getTypeName(Type type)
{
    switch (type)
    {
    case Type::None: return _("None");
    case Type::Material: return _("Material");
    case Type::EntityDef: return _("EntityDef");
    case Type::SoundShader: return _("SoundShader");
    case Type::Model: return _("Model");
    case Type::Particle: return _("Particle");
    case Type::Skin: return _("Skin");
    default:
        throw std::runtime_error("Unhandled decl type");
    }
}

}
