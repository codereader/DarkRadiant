#pragma once

#include "ideclmanager.h"
#include "SoundShader.h"

namespace sound
{

/**
 * Declaration parser capable of dealing with sound shader blocks
 */
class SoundShaderParser final :
    public decl::IDeclarationParser
{
public:
    decl::Type getDeclType() const override
    {
        return decl::Type::SoundShader;
    }

    decl::IDeclaration::Ptr parseFromBlock(const decl::DeclarationBlockSyntax& block) override
    {
        return std::make_shared<SoundShader>(block);
    }
};

}
