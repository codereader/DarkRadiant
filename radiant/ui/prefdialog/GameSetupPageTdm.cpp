#include "GameSetupPageTdm.h"

#include <set>
#include <regex>
#include "i18n.h"
#include "imodule.h"
#include "iregistry.h"
#include "itextstream.h"
#include "igame.h"

#include "wxutil/dialog/MessageBox.h"
#include "wxutil/PathEntry.h"

#include <fmt/format.h>
#include <wx/sizer.h>
#include <wx/combobox.h>
#include <wx/stattext.h>
#include <wx/panel.h>

#include "string/trim.h"
#include "registry/registry.h"
#include "string/encoding.h"
#include "os/file.h"
#include "os/path.h"
#include "os/dir.h"

namespace ui
{

namespace
{
	const std::string RKEY_FM_FOLDER_HISTORY = "user/ui/gameSetup/tdm/fmFolderHistory";
	const std::size_t HISTORY_LENGTH = 5;
}

class GameSetupPageTdm::FmFolderHistory :
	public std::list<std::string>
{
public:
	void loadFromRegistry()
	{
		xml::NodeList folders = GlobalRegistry().findXPath(std::string(RKEY_FM_FOLDER_HISTORY) + "//folder");

		for (xml::Node& node : folders)
		{
			this->push_back(node.getAttributeValue("value"));
		}
	}

	void saveToRegistry()
	{
		GlobalRegistry().deleteXPath(std::string(RKEY_FM_FOLDER_HISTORY) + "//folder");

		xml::Node folders = GlobalRegistry().createKey(RKEY_FM_FOLDER_HISTORY);

		for (const auto& folder : *this)
		{
			xml::Node node = folders.createChild("folder");
			node.setAttributeValue("value", folder);
		}
	}
};

GameSetupPageTdm::GameSetupPageTdm(wxWindow* parent, const game::IGamePtr& game) :
	GameSetupPage(parent, game),
	_fmFolderHistory(new FmFolderHistory)
{
	// Load the stored recent paths
	_fmFolderHistory->loadFromRegistry();

	wxFlexGridSizer* table = new wxFlexGridSizer(2, 2, wxSize(6, 6));
	table->AddGrowableCol(1);
	this->SetSizer(table);

	_enginePathEntry = new wxutil::PathEntry(this, true);
	_enginePathEntry->getEntryWidget()->SetMinClientSize(
		wxSize(_enginePathEntry->getEntryWidget()->GetCharWidth() * 30, -1));

	table->Add(new wxStaticText(this, wxID_ANY, _("DarkMod Path")), 0, wxALIGN_CENTRE_VERTICAL);
	table->Add(_enginePathEntry, 1, wxEXPAND);

	_enginePathEntry->getEntryWidget()->Bind(wxEVT_TEXT, [&](wxCommandEvent& ev)
	{
		populateAvailableMissionPaths();
	});

	_enginePathEntry->getEntryWidget()->SetToolTip(_("This is the path where your TheDarkMod.exe is located."));

	_missionEntry = new wxComboBox(this, wxID_ANY);
	_missionEntry->SetMinClientSize(wxSize(_missionEntry->GetCharWidth() * 30, -1));
	_missionEntry->SetToolTip(_("The FM folder name of the mission you want to work on, e.g. 'saintlucia'."));

	table->Add(new wxStaticText(this, wxID_ANY, _("Mission")), 0, wxALIGN_CENTRE_VERTICAL);
	table->Add(_missionEntry, 1, wxEXPAND);

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

bool GameSetupPageTdm::onPreSave()
{
	constructPaths();

	// Validate the engine path first, otherwise we can't do anything
	if (!os::fileOrDirExists(_config.enginePath))
	{
		return true; // proceed to normal validation routine, which will error out anyway
	}

	// Check the mod path (=mission path), if not empty
	if (!_config.modPath.empty() && !os::fileOrDirExists(_config.modPath))
	{
		// Mod name is not empty, but mod folder doesnt' exist, this might indicate that 
		// the user wants to start a new mission, ask him whether we should create the folder
		std::string missionName = _missionEntry->GetValue().ToStdString();
		std::string msg = fmt::format(_("The mission path {0} doesn't exist.\nDo you intend to start a "
			"new mission and create that folder?"), _config.modPath);

		if (wxutil::Messagebox::Show(fmt::format(_("Start a new Mission named {0}?"), missionName),
			msg, IDialog::MESSAGE_ASK, wxGetTopLevelParent(this)) == wxutil::Messagebox::RESULT_YES)
		{
			// User wants to create the path
			rMessage() << "Creating mission directory " << _config.modPath << std::endl;

			// Create the directory
			if (!os::makeDirectory(_config.modPath))
			{
				wxutil::Messagebox::Show(_("Could not create directory"), fmt::format(_("Failed to create the folder {0}"), _config.modPath),
					IDialog::MessageType::MESSAGE_ERROR, wxGetTopLevelParent(this));

				// Veto the event
				return false;
			}

			// Everything went smooth, proceed to the normal save routine 
			return true;
		}
		
		// User doesn't want to create a new mission, so veto the save event
		return false;
	}

	// Engine path is OK, mod path is empty or exists already
	return true;
}

void GameSetupPageTdm::onClose()
{
	// Add the path to the history if it's an absolute path
	if (!_config.modPath.empty() && path_is_absolute(_config.modPath.c_str()))
	{
		// Erase all existing occurrences, this moves recently used paths to the top
		_fmFolderHistory->remove(_config.modPath);

		_fmFolderHistory->push_front(_config.modPath);

		while (_fmFolderHistory->size() > HISTORY_LENGTH)
		{
			_fmFolderHistory->pop_back();
		}

		_fmFolderHistory->saveToRegistry();
	}
}

void GameSetupPageTdm::validateSettings()
{
    constructPaths();

    std::string errorMsg;

    // Validate the engine path first, otherwise we can't do anything
    if (!os::fileOrDirExists(_config.enginePath))
    {
        // Engine path doesn't exist
        errorMsg += fmt::format(_("Engine path \"{0}\" does not exist.\n"), _config.enginePath);
    }
    else
    {
        // Check if the TheDarkMod.exe file is in the right place
        fs::path darkmodExePath = _config.enginePath;

        // No engine path set so far, search the game file for default values
        const std::string ENGINE_EXECUTABLE_ATTRIBUTE =
#if defined(WIN32)
        "engine_win32"
#elif defined(__linux__) || defined (__FreeBSD__)
        "engine_linux"
#elif defined(__APPLE__)
        "engine_macos"
#else
#error "unknown platform"
#endif
        ;

        bool found = false;
        std::regex exeRegex(_game->getKeyValue(ENGINE_EXECUTABLE_ATTRIBUTE));

        os::forEachItemInDirectory(_config.enginePath, [&](const fs::path& path)
        {
            try
            {
                found |= std::regex_match(path.filename().string(), exeRegex);
            }
            catch (const std::system_error& ex)
            {
                rWarning() << "[vfs] Skipping file " << string::unicode_to_utf8(path.filename().wstring()) <<
                    " - possibly unsupported characters in filename? " <<
                    "(Exception: " << ex.what() << ")" << std::endl;
            }
        });

        if (!found)
        {
            // engine executable not present
            errorMsg += _("The engine executable(s) could not be found in the specified folder.\n");
        }
    }

    // Check the mod path (=mission path), if not empty
    if (!_config.modPath.empty() && !os::fileOrDirExists(_config.modPath))
    {
        // Path not existent => error
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
    // Remember the current value, the Clear() call will remove any text
    std::string previousMissionValue = _missionEntry->GetValue().ToStdString();

    _missionEntry->Clear();

    // Get the main engine path and calculate the FM directory path
    const fs::path enginePath = _enginePathEntry->getValue();
    if (enginePath.empty())
        return;

    const fs::path fmPath = enginePath / _fmFolder;

    // Build a sorted list of available missions. Although wxComboBox has a wxCB_SORT style,
    // we cannot use automatic sorting since we want to see the most recently used missions
    // at the top of the list.
    std::vector<std::string> sortedFMs;
    os::forEachItemInDirectory(
        fmPath.string(),
        [&](const fs::path& fmFolder) {
            // Skip the mission preview image folder
            if (fmFolder.filename() == "_missionshots")
                return;

            sortedFMs.push_back(fmFolder.filename().string());
        },
        std::nothrow
    );
    std::sort(sortedFMs.begin(), sortedFMs.end());

    // Add the sorted FMs to the combo box
    for (const auto& fm: sortedFMs) {
        _missionEntry->AppendString(fm);
    }

    // Add the history entries to the top of the list
    for (auto f = _fmFolderHistory->rbegin(); f != _fmFolderHistory->rend(); ++f)
    {
        _missionEntry->Insert(*f, 0);
    }

    // Keep the previous value after repopulation
    _missionEntry->SetValue(previousMissionValue);
}

}
