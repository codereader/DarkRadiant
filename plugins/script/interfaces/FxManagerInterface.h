#pragma once

#include "ifx.h"
#include "DeclarationManagerInterface.h"

namespace script
{

class ScriptFxAction
{
private:
    fx::IFxAction::Ptr _action;

public:
    ScriptFxAction(const fx::IFxAction::Ptr& action) :
        _action(action)
    {}
};

class ScriptFxDeclaration :
    public ScriptDeclaration
{
private:
    fx::IFxDeclaration::Ptr _fx;

public:
    ScriptFxDeclaration(const fx::IFxDeclaration::Ptr& fx) :
        ScriptDeclaration(fx),
        _fx(fx)
    {}

    std::size_t getNumActions()
    {
        return _fx ? _fx->getNumActions() : 0;
    }

    ScriptFxAction getAction(std::size_t index)
    {
        return ScriptFxAction(_fx ? _fx->getAction(index) : fx::IFxAction::Ptr());
    }

    std::string getBindTo()
    {
        return _fx ? _fx->getBindTo() : std::string();
    }
};

/**
* Exposes the GlobalFxManager interface to scripts
*/
class FxManagerInterface :
    public IScriptInterface
{
public:
    // Mapped methods
    ScriptFxDeclaration findFx(const std::string& name);

    // IScriptInterface implementation
    void registerInterface(py::module& scope, py::dict& globals) override;
};

}
