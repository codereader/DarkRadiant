#pragma once

#include "ideclmanager.h"
#include "Doom3ModelSkin.h"

namespace skins
{

class SkinCreator :
    public decl::IDeclarationCreator
{
public:
    decl::Type getDeclType() const override
    {
        return decl::Type::Skin;
    }

    decl::IDeclaration::Ptr createDeclaration(const std::string& name) override
    {
        return std::make_shared<Skin>(name);
    }
};

}
