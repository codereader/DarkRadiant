#pragma once

#include "ifx.h"
#include "DeclarationManagerInterface.h"

namespace script
{

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
