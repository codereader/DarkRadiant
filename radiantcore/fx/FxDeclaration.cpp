#include "FxDeclaration.h"

#include "string/case_conv.h"
#include "FxAction.h"

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

const char* FxDeclaration::getKeptDelimiters() const
{
    return "{}(),";
}

void FxDeclaration::onBeginParsing()
{
    _bindTo.clear();
    _actions.clear();
}

void FxDeclaration::parseFromTokens(parser::DefTokeniser& tokeniser)
{
    while (tokeniser.hasMoreTokens())
    {
        auto token = tokeniser.nextToken();
        string::to_lower(token);

        if (token == "bindto")
        {
            _bindTo = tokeniser.nextToken();
        }
        else if (token == "{")
        {
            // An opening brace indicates an action, proceed
            auto action = std::make_shared<FxAction>(*this);
            action->parseFromTokens(tokeniser);

            _actions.emplace_back(std::move(action));
        }
    }
}

}
