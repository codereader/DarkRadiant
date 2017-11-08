#include "GameSetupPageIdTech.h"

#include "i18n.h"
#include "imodule.h"
#include "igame.h"

#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/panel.h>

#include "registry/Widgets.h"

namespace ui
{

GameSetupPageIdTech::GameSetupPageIdTech(wxWindow* parent) :
	GameSetupPage(parent)
{
	wxFlexGridSizer* table = new wxFlexGridSizer(3, 2, wxSize(6, 6));
	this->SetSizer(table);

	table->Add(new wxStaticText(this, wxID_ANY, _("Engine Path")), 0, wxALIGN_CENTRE_VERTICAL);
	table->Add(createEntry(RKEY_ENGINE_PATH), 0);

	table->Add(new wxStaticText(this, wxID_ANY, _("Mod (fs_game)")), 0, wxALIGN_CENTRE_VERTICAL);
	table->Add(createEntry(RKEY_FS_GAME), 0);

	table->Add(new wxStaticText(this, wxID_ANY, _("Mod Base (fs_game_base, optional)")), 0, wxALIGN_CENTRE_VERTICAL);
	table->Add(createEntry(RKEY_FS_GAME_BASE), 0);
}

const char* GameSetupPageIdTech::TYPE()
{
	return "idTechGeneric";
}

const char* GameSetupPageIdTech::getType()
{
	return TYPE();
}

void GameSetupPageIdTech::validateSettings()
{

}

void GameSetupPageIdTech::saveSettings()
{

}

wxWindow* GameSetupPageIdTech::createEntry(const std::string& registryKey)
{
	wxTextCtrl* entryWidget = new wxTextCtrl(this, wxID_ANY);

	int minChars = static_cast<int>(std::max(_registryBuffer.get(registryKey).size(), std::size_t(30)));
	entryWidget->SetMinClientSize(wxSize(entryWidget->GetCharWidth() * minChars, -1));

	// Connect the registry key to the newly created input field
	registry::bindWidgetToBufferedKey(entryWidget, registryKey, _registryBuffer, _resetValuesSignal);

	return entryWidget;
}

}
