#pragma once

#include <stdexcept>
#include <string>

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
    Table,
    EntityDef,
    SoundShader,
    ModelDef,
    Particle,
    Skin,
    Fx,

    // These are used in unit tests only
    TestDecl,
    TestDecl2,
};

inline std::string getTypeName(Type type)
{
    switch (type)
    {
    case Type::Undetermined: return "Undetermined";
    case Type::None: return "None";
    case Type::Material: return "Material";
    case Type::Table: return "Table";
    case Type::EntityDef: return "EntityDef";
    case Type::SoundShader: return "SoundShader";
    case Type::ModelDef: return "ModelDef";
    case Type::Particle: return "Particle";
    case Type::Skin: return "Skin";
    case Type::Fx: return "Fx";
    case Type::TestDecl: return "TestDecl";
    case Type::TestDecl2: return "TestDecl2";
    default:
        throw std::runtime_error("Unhandled decl type");
    }
}

}
