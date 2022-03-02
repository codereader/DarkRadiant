#pragma once

#include <stdexcept>
#include <string>

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
    case Type::None: return "None";
    case Type::Material: return "Material";
    case Type::EntityDef: return "EntityDef";
    case Type::SoundShader: return "SoundShader";
    case Type::Model: return "Model";
    case Type::Particle: return "Particle";
    case Type::Skin: return "Skin";
    default:
        throw std::runtime_error("Unhandled decl type");
    }
}

}
