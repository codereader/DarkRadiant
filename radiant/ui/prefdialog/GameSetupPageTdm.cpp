#include "GameSetupPageTdm.h"

#include "i18n.h"
#include "imodule.h"
#include "igame.h"

#include "wxutil/PathEntry.h"

#include <fmt/format.h>
#include <wx/sizer.h>
#include <wx/combobox.h>
#include <wx/stattext.h>
#include <wx/panel.h>

#include "string/trim.h"
#include "os/file.h"
#include "os/path.h"
#include "os/dir.h"

namespace ui
{

GameSetupPageTdm::GameSetupPageTdm(wxWindow* parent, const game::IGamePtr& game) :
	GameSetupPage(parent, game),
	_missionEntry(nullptr),
	_enginePathEntry(nullptr)
{
	wxFlexGridSizer* table = new wxFlexGridSizer(2, 2, wxSize(6, 6));
	this->SetSizer(table);

	_enginePathEntry = new wxutil::PathEntry(this, true);
	_enginePathEntry->getEntryWidget()->SetMinClientSize(
		wxSize(_enginePathEntry->getEntryWidget()->GetCharWidth() * 30, -1));

	table->Add(new wxStaticText(this, wxID_ANY, _("Darkmod Path")), 0, wxALIGN_CENTRE_VERTICAL);
	table->Add(_enginePathEntry, 0);

	_enginePathEntry->getEntryWidget()->Bind(wxEVT_TEXT, [&](wxCommandEvent& ev)
	{
		populateAvailableMissionPaths();
	});

	_missionEntry = new wxComboBox(this, wxID_ANY);
	_missionEntry->SetMinClientSize(wxSize(_missionEntry->GetCharWidth() * 30, -1));
	table->Add(new wxStaticText(this, wxID_ANY, _("Mission")), 0, wxALIGN_CENTRE_VERTICAL);
	table->Add(_missionEntry, 0);

	// Determine the fms folder name where the missions are stored, defaults to "fms/"
	_fmFolder = "fms/";
	
	xml::NodeList nodes = game->getLocalXPath("/gameSetup/missionFolder");

	if (!nodes.empty())
	{
		std::string value = nodes[0].getAttributeValue("value");

		if (!value.empty())
		{
			_fmFolder = os::standardPathWithSlash(value);
		}
	}

	populateAvailableMissionPaths();
}

const char* GameSetupPageTdm::TYPE()
{
	return "TDM";
}

const char* GameSetupPageTdm::getType()
{
	return TYPE();
}

void GameSetupPageTdm::validateSettings()
{
	constructPaths();

	std::string errorMsg;

	if (!os::fileOrDirExists(_config.enginePath))
	{
		// Engine path doesn't exist
		errorMsg += fmt::format(_("Engine path \"{0}\" does not exist.\n"), _config.enginePath);
	}

	// Check the mod path (=mission path), if not empty
	if (!_config.modPath.empty() && !os::fileOrDirExists(_config.modPath))
	{
		// Mod name is not empty, but mod folder doesnt' exist
		errorMsg += fmt::format(_("The mission path \"{0}\" does not exist.\n"), _config.modPath);
	}

	if (!errorMsg.empty())
	{
		throw GameSettingsInvalidException(errorMsg);
	}
}

void GameSetupPageTdm::onPageShown()
{
	// Load the values from the registry if the controls are still empty
	if (_enginePathEntry->getValue().empty())
	{
		_config.enginePath = registry::getValue<std::string>(RKEY_ENGINE_PATH);
		_config.enginePath = os::standardPathWithSlash(_config.enginePath);

		// If the engine path is empty, consult the .game file for a fallback value
		if (_config.enginePath.empty())
		{
			// No engine path set so far, search the game file for default values
			const std::string ENGINEPATH_ATTRIBUTE =
#if defined(WIN32)
				"enginepath_win32"
#elif defined(__linux__) || defined (__FreeBSD__)
				"enginepath_linux"
#elif defined(__APPLE__)
				"enginepath_macos"
#else
#error "unknown platform"
#endif
				;

			_config.enginePath = os::standardPathWithSlash(_game->getKeyValue(ENGINEPATH_ATTRIBUTE));
		}

		_enginePathEntry->setValue(_config.enginePath);
	}

	if (_missionEntry->GetValue().empty())
	{
		// Check if we have a valid mod path
		_config.modPath = registry::getValue<std::string>(RKEY_MOD_PATH);

		if (!_config.modPath.empty())
		{
			// Extract the mission name from the absolute mod path, if possible
			std::string fmPath = _config.enginePath + _fmFolder;

			std::string fmName = os::getRelativePath(_config.modPath, fmPath);
			string::trim(fmName, "/");

			_missionEntry->SetValue(fmName);
		}
	}
}

void GameSetupPageTdm::constructPaths()
{
	_config.enginePath = _enginePathEntry->getEntryWidget()->GetValue().ToStdString();

	// Make sure it's a well formatted path
	_config.enginePath = os::standardPathWithSlash(_config.enginePath);
	
	// No mod base path necessary
	_config.modBasePath.clear();

	// Load the mission folder name from the registry
	std::string fmName = _missionEntry->GetValue().ToStdString();

	if (!fmName.empty())
	{
		// greebo: #3480 check if the mod path is absolute. If not, append it to the engine path
		_config.modPath = fs::path(fmName).is_absolute() ? fmName : _config.enginePath + _fmFolder + string::trim_copy(fmName, "/");

		// Normalise the path as last step
		_config.modPath = os::standardPathWithSlash(_config.modPath);
	}
	else
	{
		// No mission name, no modpath
		_config.modPath.clear();
	}
}

void GameSetupPageTdm::populateAvailableMissionPaths()
{
	_missionEntry->Clear();

	fs::path enginePath = _enginePathEntry->getValue();

	if (enginePath.empty())
	{
		return;
	}

	try
	{
		fs::path fmPath = enginePath / "fms";

		os::foreachItemInDirectory(fmPath.string(), [&](const fs::path& fmFolder)
		{
			_missionEntry->AppendString(fmFolder.filename().string());
		});
	}
	catch (const os::DirectoryNotFoundException&)
	{}
}

}
