#include "SoundChooser.h"

#include "i18n.h"
#include "ui/UserInterfaceModule.h"

#include <wx/stattext.h>

namespace ui
{

// Constructor
SoundChooser::SoundChooser(wxWindow* parent) :
    DeclarationSelectorDialog(decl::Type::SoundShader, _("Choose sound"), "SoundChooser", parent),
    _selector(nullptr)
{
    _selector = new SoundShaderSelector(this);
    SetSelector(_selector);

    auto dblClickHint = new wxStaticText(this, wxID_ANY, _("Ctrl + Double Click on treeview items for quick play"));
    AddItemToBottomRow(dblClickHint);
}

void SoundChooser::loadSoundShaders()
{
    _selector->Populate();
}

void SoundChooser::_onReloadSounds(wxCommandEvent& ev)
{
    SetSelectedDeclName({});

    // Send the command to the SoundManager
    // After parsing it will fire the sounds reloaded signal => onShadersReloaded()
    GlobalCommandSystem().executeCommand("ReloadSounds");
}

void SoundChooser::onShadersReloaded()
{
    // This signal is fired by the SoundManager, possibly from a non-UI thread
    GetUserInterfaceModule().dispatch([this]()
    {
        loadSoundShaders();
    });
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
