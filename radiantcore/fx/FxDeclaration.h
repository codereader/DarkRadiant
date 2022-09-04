#pragma once

#include <vector>
#include "ifx.h"
#include "decl/DeclarationBase.h"

namespace fx
{

class FxDeclaration :
    public decl::DeclarationBase<IFxDeclaration>
{
private:
    std::vector<IFxAction::Ptr> _actions;
    std::string _bindTo;

public:
    FxDeclaration(const std::string& name);

    std::size_t getNumActions() override;
    IFxAction::Ptr getAction(std::size_t index) override;
    std::string getBindTo() override;

protected:
    const char* getKeptDelimiters() const override;
    void onBeginParsing() override;
    void parseFromTokens(parser::DefTokeniser& tokeniser) override;
};

}
