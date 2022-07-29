#pragma once

#include <sigc++/connection.h>
#include "ideclmanager.h"
#include "i18n.h"
#include "fmt/format.h"
#include "DefinitionView.h"
#include "SourceView.h"

namespace wxutil
{

/**
 * DefinitionView implementation specialising on Declarations
 */
class DeclarationSourceView :
    public DefinitionView
{
private:
    decl::IDeclaration::Ptr _decl;

    sigc::connection _declChangedConn;

public:
    DeclarationSourceView(const decl::Type& type, wxWindow* parent) :
        DefinitionView("", parent)
    {
        // Pick the correct source view control based on the given type
        switch (type)
        {
        case decl::Type::SoundShader:
            addSourceView(new D3SoundShaderSourceViewCtrl(getMainPanel()));
            break;
        case decl::Type::Material:
            addSourceView(new D3MaterialSourceViewCtrl(getMainPanel()));
            break;
        default:
            addSourceView(new D3DeclarationViewCtrl(getMainPanel()));
        }
    }

    ~DeclarationSourceView()
    {
        _declChangedConn.disconnect();
    }

    void setDeclaration(const decl::IDeclaration::Ptr& decl)
    {
        _declChangedConn.disconnect();

        _decl = decl;

        if (_decl)
        {
            _declChangedConn = _decl->signal_DeclarationChanged().connect(
                sigc::mem_fun(*this, &DeclarationSourceView::update)
            );
        }

        update();
        updateTitle();
    }

    void setDeclaration(decl::Type type, const std::string& declName)
    {
        setDeclaration(GlobalDeclarationManager().findDeclaration(type, declName));
    }

protected:
    bool isEmpty() const override
    {
        return !_decl;
    }

    std::string getDeclName() override
    {
        return _decl ? _decl->getDeclName() : "";
    }

    std::string getDeclFileName() override
    {
        return _decl ? _decl->getDeclFilePath() : "";
    }

    std::string getDefinition() override
    {
        return _decl ? _decl->getBlockSyntax().contents : "";
    }

private:
    void updateTitle()
    {
        SetTitle(fmt::format(_("Declaration Source: {0}"), !isEmpty() ? _decl->getDeclName() : ""));
    }
};

}
