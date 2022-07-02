#pragma once

#include "ideclmanager.h"

#include "Doom3ModelDef.h"
#include "EntityClass.h"

namespace eclass
{

class EntityDefCreator :
    public decl::IDeclarationCreator
{
public:
    decl::Type getDeclType() const override
    {
        return decl::Type::EntityDef;
    }

    decl::IDeclaration::Ptr createDeclaration(const std::string& name) override
    {
        return std::make_shared<EntityClass>(name);
    }
};

class ModelDefCreator :
    public decl::IDeclarationCreator
{
public:
    decl::Type getDeclType() const override
    {
        return decl::Type::ModelDef;
    }

    decl::IDeclaration::Ptr createDeclaration(const std::string& name) override
    {
        return std::make_shared<Doom3ModelDef>(name);
    }
};

}
