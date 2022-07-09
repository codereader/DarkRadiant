#pragma once

#include <stdexcept>
#include <string>
#include "i18n.h"

// Some X11 headers are defining this
#ifdef None
#undef None
#endif

namespace decl
{

// Enumeration of declaration types supported by DarkRadiant
enum class Type
{
    Undetermined = -2,
    None = -1,
    Material = 0,
    EntityDef,
    SoundShader,
    Model,
    ModelDef,
    Particle,
    Skin,

    // These are used in unit tests only
    TestDecl,
    TestDecl2,
};

inline std::string getTypeName(Type type)
{
    switch (type)
    {
    case Type::Undetermined: return _("Undetermined");
    case Type::None: return _("None");
    case Type::Material: return _("Material");
    case Type::EntityDef: return _("EntityDef");
    case Type::SoundShader: return _("SoundShader");
    case Type::Model: return _("Model");
    case Type::ModelDef: return _("ModelDef");
    case Type::Particle: return _("Particle");
    case Type::Skin: return _("Skin");
    default:
        throw std::runtime_error("Unhandled decl type");
    }
}

}
