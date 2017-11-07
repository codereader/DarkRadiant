#pragma once

#include <stdexcept>
#include <functional>
#include <map>
#include <wx/panel.h>

namespace ui
{

class GameSettingsInvalidException :
	public std::logic_error
{
public:
	GameSettingsInvalidException(const std::string& msg) :
		std::logic_error(msg)
	{}
};

/**
* Represents a page in the Game Setup dialog. 
*
* Such a page needs to collect the relevant paths and settings
* from the user and validate them.
*/
class GameSetupPage :
	public wxPanel
{
public:
	GameSetupPage(wxWindow* parent);

	virtual ~GameSetupPage() {}

	// Returns the type ID of this page, as referenced by the .game file
	// e.g. <dialog type="idTechGeneric" />
	virtual const char* getType() = 0;

	// Validates the settings on the page. This should throw a
	// GameSettingsInvalidException in case something is not correct.
	virtual void validateSettings() = 0;

	// Saves the settings to the registry
	virtual void saveSettings() = 0;

public:
	typedef std::function<GameSetupPage*(wxWindow*)> CreateInstanceFunc;

	// Base class keeps a map of registered pages
	typedef std::map<std::string, GameSetupPage::CreateInstanceFunc> GameSetupPages;
	static GameSetupPages _registeredPages;

	static void EnsureDefaultPages();

	// Creates the setup page instance for the given type
	static GameSetupPage* CreatePageForType(const std::string& type, wxWindow* parent);
};

}
