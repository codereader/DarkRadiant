#include "GameSetupPageIdTech.h"

#include "i18n.h"
#include "imodule.h"
#include "igame.h"

#include "wxutil/PathEntry.h"

#include <fmt/format.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/panel.h>

#include "os/file.h"
#include "registry/Widgets.h"

namespace ui
{

GameSetupPageIdTech::GameSetupPageIdTech(wxWindow* parent) :
	GameSetupPage(parent),
	_fsGameEntry(nullptr),
	_fsGameBaseEntry(nullptr),
	_enginePathEntry(nullptr)
{
	wxFlexGridSizer* table = new wxFlexGridSizer(3, 2, wxSize(6, 6));
	this->SetSizer(table);

	_enginePathEntry = createPathEntry(RKEY_ENGINE_PATH);
	table->Add(new wxStaticText(this, wxID_ANY, _("Engine Path")), 0, wxALIGN_CENTRE_VERTICAL);
	table->Add(_enginePathEntry, 0);

	_fsGameEntry = createEntry(RKEY_FS_GAME);
	table->Add(new wxStaticText(this, wxID_ANY, _("Mod (fs_game)")), 0, wxALIGN_CENTRE_VERTICAL);
	table->Add(_fsGameEntry, 0);

	_fsGameBaseEntry = createEntry(RKEY_FS_GAME_BASE);
	table->Add(new wxStaticText(this, wxID_ANY, _("Mod Base (fs_game_base, optional)")), 0, wxALIGN_CENTRE_VERTICAL);
	table->Add(_fsGameBaseEntry, 0);

	// TODO: Derive some default values for the paths unless they're already set
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
	constructPaths();
	
	std::string errorMsg;

	if (!os::fileOrDirExists(_enginePath))
	{
		// Engine path doesn't exist
		errorMsg += fmt::format(_("Engine path \"{0}\" does not exist.\n"), _enginePath);
	}

	// Check the mod base path, if not empty
	if (!_modBasePath.empty() && !os::fileOrDirExists(_modBasePath))
	{
		// Mod base name is not empty, but folder doesnt' exist
		errorMsg += fmt::format(_("The mod base path \"{0}\" does not exist.\n"), _modBasePath);
	}

	// Check the mod path, if not empty
	if (!_modPath.empty() && !os::fileOrDirExists(_modPath))
	{
		// Mod name is not empty, but mod folder doesnt' exist
		errorMsg += fmt::format(_("The mod path \"{0}\" does not exist.\n"), _modPath);
	}

	if (!errorMsg.empty())
	{
		throw GameSettingsInvalidException(errorMsg);
	}
}

std::string GameSetupPageIdTech::getEnginePath()
{
	return _enginePath;
}

std::string GameSetupPageIdTech::getModBasePath()
{
	return _modBasePath;
}

std::string GameSetupPageIdTech::getModPath()
{
	return _modPath;
}

void GameSetupPageIdTech::constructPaths()
{
	_enginePath = _enginePathEntry->getEntryWidget()->GetValue().ToStdString();

	// Make sure it's a well formatted path
	_enginePath = os::standardPathWithSlash(_enginePath);

	// Load the fsGame and fsGameBase from the registry
	std::string fsGame = _fsGameEntry->GetValue().ToStdString();
	std::string fsGameBase = _fsGameBaseEntry->GetValue().ToStdString();

	if (!fsGameBase.empty())
	{
		// greebo: #3480 check if the mod base path is absolute. If not, append it to the engine path
		_modBasePath = fs::path(fsGameBase).is_absolute() ? fsGameBase : _enginePath + fsGameBase;

		// Normalise the path as last step
		_modBasePath = os::standardPathWithSlash(_modBasePath);
	}
	else
	{
		// No fs_game_base, no mod base path
		_modBasePath = "";
	}

	if (!fsGame.empty())
	{
		// greebo: #3480 check if the mod path is absolute. If not, append it to the engine path
		_modPath = fs::path(fsGame).is_absolute() ? fsGame : _enginePath + fsGame;

		// Normalise the path as last step
		_modPath = os::standardPathWithSlash(_modPath);
	}
	else
	{
		// No fs_game, no modpath
		_modPath = "";
	}
}

wxTextCtrl* GameSetupPageIdTech::createEntry(const std::string& registryKey)
{
	wxTextCtrl* entryWidget = new wxTextCtrl(this, wxID_ANY);

	int minChars = static_cast<int>(std::max(_registryBuffer.get(registryKey).size(), std::size_t(30)));
	entryWidget->SetMinClientSize(wxSize(entryWidget->GetCharWidth() * minChars, -1));

	// Connect the registry key to the newly created input field
	registry::bindWidgetToBufferedKey(entryWidget, registryKey, _registryBuffer, _resetValuesSignal);

	return entryWidget;
}

wxutil::PathEntry* GameSetupPageIdTech::createPathEntry(const std::string& registryKey)
{
	wxutil::PathEntry* entry = new wxutil::PathEntry(this, true);

	// Connect the registry key to the newly created input field
	registry::bindWidgetToBufferedKey(entry->getEntryWidget(), registryKey, _registryBuffer, _resetValuesSignal);

	int minChars = static_cast<int>(std::max(GlobalRegistry().get(registryKey).size(), std::size_t(30)));

	entry->getEntryWidget()->SetMinClientSize(
		wxSize(entry->getEntryWidget()->GetCharWidth() * minChars, -1));

	// Initialize entry
	entry->setValue(registry::getValue<std::string>(registryKey));

	return entry;
}

}
