#pragma once

#include "ifx.h"
#include "decl/DeclarationBase.h"

namespace fx
{

class FxDeclaration :
    public decl::DeclarationBase<IFxDeclaration>
{
public:
    FxDeclaration(const std::string& name);

protected:
    void parseFromTokens(parser::DefTokeniser& tokeniser) override;
};

}
