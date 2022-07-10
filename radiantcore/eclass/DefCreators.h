#pragma once

#include "ideclmanager.h"

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
        return EntityClass::CreateDefault(name);
    }
};

}
