#include "FxChooser.h"

#include "i18n.h"
#include "debugging/ScopedDebugTimer.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "wxutil/decl/DeclarationSelector.h"

namespace ui
{

class FxSelector :
    public wxutil::DeclarationSelector
{
private:
    constexpr static const char* const FX_ICON = "icon_fx.png";

public:
    FxSelector(wxWindow* parent) :
        DeclarationSelector(parent, decl::Type::Fx)
    {
        Populate();
    }

    void Populate() override
    {
        PopulateTreeView(std::make_shared<wxutil::ThreadedDeclarationTreePopulator>(decl::Type::Fx, GetColumns(), FX_ICON));
    }
};

FxChooser::FxChooser(wxWindow* parent) :
    DeclarationSelectorDialog(decl::Type::SoundShader, _("Choose FX Declaration"), "FxChooser", parent)
{
    SetSelector(new FxSelector(this));
}

std::string FxChooser::ChooseDeclaration(const std::string& preselected)
{
    FxChooser instance;

    if (!preselected.empty())
    {
        instance.SetSelectedDeclName(preselected);
    }

    if (instance.ShowModal() == wxID_OK)
    {
        return instance.GetSelectedDeclName();
    }

    return ""; // Empty selection on cancel
}

}
