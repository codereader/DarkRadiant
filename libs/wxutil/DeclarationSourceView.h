#pragma once

#include "ideclmanager.h"
#include "i18n.h"
#include "fmt/format.h"
#include "DefinitionView.h"
#include "SourceView.h"

namespace wxutil
{

class DeclarationSourceView :
    public DefinitionView
{
private:
    decl::IDeclaration::Ptr _decl;

public:
    DeclarationSourceView(const decl::IDeclaration::Ptr& decl, wxWindow* parent) :
        DefinitionView(fmt::format(_("Declaration Source: {0}"), decl->getDeclName()), parent),
        _decl(decl)
    {
        // Pick the correct source view control based on the given type
        switch (_decl->getDeclType())
        {
        case decl::Type::SoundShader:
            addSourceView(new D3SoundShaderSourceViewCtrl(getMainPanel()));
            break;
        default:
            addSourceView(new D3DeclarationViewCtrl(getMainPanel()));
        }
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
};

}
