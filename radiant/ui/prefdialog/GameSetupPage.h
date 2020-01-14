#pragma once

#include "igame.h"
#include <stdexcept>
#include <functional>
#include <map>
#include <wx/panel.h>
#include "settings/GameConfiguration.h"

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
protected:
	// The game we're working on
	game::IGamePtr _game;

	// The configuration. This structure should be filled in
	// as a result of this setup page. The game type member is 
	// already assigned by this base class.
	game::GameConfiguration _config;

public:
	GameSetupPage(wxWindow* parent, const game::IGamePtr& game);

	virtual ~GameSetupPage() {}

	// Returns the type ID of this page, as referenced by the .game file
	// e.g. <dialog type="idTechGeneric" />
	virtual const char* getType() = 0;

	// Validates the settings on the page. This should throw a
	// GameSettingsInvalidException in case something is not correct.
	virtual void validateSettings() = 0;

	// Called by the owning GameSetupDialog when this page is selected
	virtual void onPageShown() = 0;

	// Fired before the dialog is about to run the save routine
	// Override and return false here to veto the event
	virtual bool onPreSave()
	{
		return true; // return true by default, don't veto
	}

	// Overridable event which is fired before the page is closed
	virtual void onClose()
	{}

	// Accessor to the result config structure
	virtual const game::GameConfiguration& getConfiguration();

public:
	typedef std::function<GameSetupPage*(wxWindow*, const game::IGamePtr&)> CreateInstanceFunc;

	// Base class keeps a map of registered pages
	typedef std::map<std::string, GameSetupPage::CreateInstanceFunc> GameSetupPages;
	static GameSetupPages _registeredPages;

	static void EnsureDefaultPages();

	// Creates the setup page instance for the given type
	static GameSetupPage* CreatePageForGame(const game::IGamePtr& game, wxWindow* parent);
};

}
