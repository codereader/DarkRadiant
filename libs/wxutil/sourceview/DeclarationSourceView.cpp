#include "DeclarationSourceView.h"

#include "i18n.h"
#include "fmt/format.h"

#include "SourceView.h"

// Some X11 headers are defining this
#ifdef None
#undef None
#endif

namespace wxutil
{

DeclarationSourceView::DeclarationSourceView(wxWindow* parent) :
    DefinitionView("", parent),
    _activeSourceViewType(decl::Type::Undetermined)
{
    updateSourceView();
}

DeclarationSourceView::~DeclarationSourceView()
{
    _declChangedConn.disconnect();
}

void DeclarationSourceView::setDeclaration(const decl::IDeclaration::Ptr& decl)
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

void DeclarationSourceView::setDeclaration(decl::Type type, const std::string& declName)
{
    setDeclaration(GlobalDeclarationManager().findDeclaration(type, declName));
}

bool DeclarationSourceView::isEmpty() const
{
    return !_decl;
}

std::string DeclarationSourceView::getDeclName()
{
    return _decl ? _decl->getDeclName() : "";
}

std::string DeclarationSourceView::getDeclFileName()
{
    return _decl ? _decl->getDeclFilePath() : "";
}

std::string DeclarationSourceView::getDefinition()
{
    return _decl ? _decl->getBlockSyntax().contents : "";
}

void DeclarationSourceView::updateTitle()
{
    SetTitle(fmt::format(_("Declaration Source: {0}"), !isEmpty() ? _decl->getDeclName() : ""));
}

void DeclarationSourceView::updateSourceView()
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
    case decl::Type::Particle:
        setSourceView(new D3ParticleSourceViewCtrl(getMainPanel()));
        break;
    case decl::Type::ModelDef:
        setSourceView(new D3ModelDefSourceViewCtrl(getMainPanel()));
        break;
    default:
        setSourceView(new D3DeclarationViewCtrl(getMainPanel()));
    }
}

}
