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
    decl::Type _activeSourceViewType;

    sigc::connection _declChangedConn;

public:
    DeclarationSourceView(wxWindow* parent) :
        DefinitionView("", parent),
        _activeSourceViewType(decl::Type::Undetermined)
    {
        updateSourceView();
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

        updateSourceView();
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

    void updateSourceView()
    {
        auto newType = _decl ? _decl->getDeclType() : decl::Type::None;

        if (newType == _activeSourceViewType) return;

        _activeSourceViewType = newType;

        // Pick the correct source view control based on the active type
        switch (newType)
        {
        case decl::Type::SoundShader:
            setSourceView(new D3SoundShaderSourceViewCtrl(getMainPanel()));
            break;
        case decl::Type::Material:
            setSourceView(new D3MaterialSourceViewCtrl(getMainPanel()));
            break;
        default:
            setSourceView(new D3DeclarationViewCtrl(getMainPanel()));
        }
    }
};

}
