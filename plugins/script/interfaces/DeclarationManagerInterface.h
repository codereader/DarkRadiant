#pragma once

#include "iscriptinterface.h"
#include "ideclmanager.h"

namespace script
{

// Wrapper class to represent an IDeclaration object in Python
class ScriptDeclaration
{
private:
    decl::IDeclaration::Ptr _decl;

public:
    ScriptDeclaration(const decl::IDeclaration::Ptr& decl) :
        _decl(decl)
    {}

    // TODO
};

/**
* Exposes the GlobalDeclarationManager interface to scripts
*/
class DeclarationManagerInterface :
    public IScriptInterface
{
public:
    // Mapped methods
    ScriptDeclaration findDeclaration(decl::Type type, const std::string& name);

    // IScriptInterface implementation
    void registerInterface(py::module& scope, py::dict& globals) override;
};

}
