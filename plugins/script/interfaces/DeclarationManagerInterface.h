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

    const std::string& getDeclName() const
    {
        static std::string _emptyName;
        return _decl ? _decl->getDeclName() : _emptyName;
    }

    decl::Type getDeclType() const
    {
        return _decl ? _decl->getDeclType() : decl::Type::None;
    }

    const decl::DeclarationBlockSyntax& getBlockSyntax()
    {
        static decl::DeclarationBlockSyntax _emptySyntax;
        return _decl ? _decl->getBlockSyntax() : _emptySyntax;
    }

    void setBlockSyntax(const decl::DeclarationBlockSyntax& block)
    {
        if (_decl)
        {
            _decl->setBlockSyntax(block);
        }
    }

    std::string getDeclFilePath() const
    {
        return _decl ? _decl->getDeclFilePath() : "";
    }
};

class DeclarationVisitor
{
public:
    virtual ~DeclarationVisitor() {}
    virtual void visit(const decl::IDeclaration::Ptr& declaration) = 0;
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
    ScriptDeclaration findOrCreateDeclaration(decl::Type type, const std::string& name);
    void foreachDeclaration(decl::Type type, DeclarationVisitor& visitor);

    // IScriptInterface implementation
    void registerInterface(py::module& scope, py::dict& globals) override;
};

}
