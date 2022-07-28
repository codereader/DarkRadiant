#pragma once

#include "iscriptinterface.h"
#include "ideclmanager.h"
#include "os/path.h"

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

    const decl::IDeclaration::Ptr& get() const
    {
        return _decl;
    }

    bool isNull() const
    {
        return !_decl;
    }

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

    void setDeclFilePath(const std::string& folder, const std::string& filename)
    {
        if (!_decl) return;

        _decl->setFileInfo(vfs::FileInfo(os::standardPathWithSlash(folder), filename, vfs::Visibility::NORMAL));
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
    bool renameDeclaration(decl::Type type, const std::string& oldName, const std::string& newName);
    void removeDeclaration(decl::Type type, const std::string& name);
    void reloadDeclarations();
    void saveDeclaration(const ScriptDeclaration& decl);

    // IScriptInterface implementation
    void registerInterface(py::module& scope, py::dict& globals) override;
};

}
