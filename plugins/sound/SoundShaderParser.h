#pragma once

#include "ideclmanager.h"
#include "SoundShader.h"

namespace sound
{

/**
 * Declaration creator capable of dealing with sound shader blocks
 */
class SoundShaderParser final :
    public decl::IDeclarationCreator
{
public:
    decl::Type getDeclType() const override
    {
        return decl::Type::SoundShader;
    }

    decl::IDeclaration::Ptr createDeclaration(const std::string& name) override
    {
        return std::make_shared<SoundShader>(name);
    }
};

}
