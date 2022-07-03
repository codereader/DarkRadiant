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
        // greebo: Changed fallback behaviour when unknown entites are encountered to isFixedSize == FALSE
        // so that brushes of unknown entites don't get lost (issue #240)
        return EntityClass::CreateDefault(name, false);
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
