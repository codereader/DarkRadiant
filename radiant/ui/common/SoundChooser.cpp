#include "SoundChooser.h"

#include "i18n.h"
#include <wx/stattext.h>

#include "ui/common/SoundShaderSelector.h"

namespace ui
{

SoundChooser::SoundChooser(wxWindow* parent) :
    DeclarationSelectorDialog(decl::Type::SoundShader, _("Choose sound"), "SoundChooser", parent)
{
    SetSelector(new SoundShaderSelector(this));
    AddItemToBottomRow(new wxStaticText(this, wxID_ANY, _("Ctrl + Double Click on treeview items for quick play")));
}

std::string SoundChooser::chooseResource(const std::string& shaderToPreselect)
{
	if (!shaderToPreselect.empty())
	{
		SetSelectedDeclName(shaderToPreselect);
	}

    if (ShowModal() == wxID_OK)
    {
        return GetSelectedDeclName();
    }

    return ""; // Empty selection on cancel
}

void SoundChooser::destroyDialog()
{
	Destroy();
}

} // namespace
