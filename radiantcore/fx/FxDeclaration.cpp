#include "FxDeclaration.h"

namespace fx
{

FxDeclaration::FxDeclaration(const std::string& name) :
    DeclarationBase<IFxDeclaration>(decl::Type::Fx, name)
{}

void FxDeclaration::parseFromTokens(parser::DefTokeniser& tokeniser)
{
    
}

}
