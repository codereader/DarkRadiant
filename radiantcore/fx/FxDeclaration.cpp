#include "FxDeclaration.h"

namespace fx
{

FxDeclaration::FxDeclaration(const std::string& name) :
    DeclarationBase<IFxDeclaration>(decl::Type::Fx, name)
{}

std::size_t FxDeclaration::getNumActions()
{
    ensureParsed();
    return _actions.size();
}

IFxAction::Ptr FxDeclaration::getAction(std::size_t index)
{
    ensureParsed();
    return _actions.at(index);
}

std::string FxDeclaration::getBindTo()
{
    ensureParsed();
    return _bindTo;
}

void FxDeclaration::onBeginParsing()
{
    _bindTo.clear();
    _actions.clear();
}

void FxDeclaration::parseFromTokens(parser::DefTokeniser& tokeniser)
{
    
}

}
