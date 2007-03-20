#include "GameDialog.h"

#include <gtk/gtkcontainer.h>

#include "os/dir.h"
#include "os/file.h"

#include "environment.h"
#include "preferencedictionary.h"
#include "preferences.h"
#include "error.h"

#include "GameDescription.h"
#include "GameFileLoader.h"

extern PreferenceDictionary g_global_preferences;

	namespace {
		const std::string RKEY_GAME_TYPE = "user/game/type";
	}

namespace ui {

GameDialog::~GameDialog() {
    // free all the game descriptions
    std::list < GameDescription * >::iterator iGame;
    for (iGame = mGames.begin(); iGame != mGames.end(); ++iGame) {
        delete(*iGame);
        *iGame = 0;
    }
    if (GetWidget() != 0) {
        Destroy();
    }
}

void GameDialog::LoadPrefs()
{
	// load global .pref file
	std::string globalPrefFile = g_Preferences._globalPrefPath + "global.pref";
	globalOutputStream() << "loading global preferences from " << makeQuoted(globalPrefFile.c_str()) << "\n";

	if (!Preferences_Load(g_global_preferences, globalPrefFile.c_str())) {
		globalOutputStream() << "failed to load global preferences from " << globalPrefFile.c_str() << "\n";
	}
}

void GameDialog::SavePrefs()
{
	std::string globalPrefFile = g_Preferences._globalPrefPath + "global.pref";

	globalOutputStream() << "saving global preferences to " << globalPrefFile.c_str() << "\n";

	if (!Preferences_Save_Safe(g_global_preferences, globalPrefFile.c_str())) {
		globalOutputStream() << "failed to save global preferences to " << globalPrefFile.c_str() << "\n";
	}
}

void GameDialog::DoGameDialog()
{
  // show the UI
  DoModal();

  // we save the prefs file
  SavePrefs();
}

void GameDialog::keyChanged() {
	 _currentGameDescription = GameDescriptionForRegistryKey();
}

void GameDialog::CreateGlobalFrame(PrefPage* page)
{
	ComboBoxValueList gameList;

	for (std::list<GameDescription*>::iterator i = mGames.begin(); i != mGames.end(); ++i) {
		gameList.push_back((*i)->getRequiredKeyValue("name"));
	}
	page->appendCombo("Select Game", RKEY_GAME_TYPE, gameList); 
}

GtkWindow* GameDialog::BuildDialog()
{	
  GtkFrame* frame = create_dialog_frame("Game settings", GTK_SHADOW_ETCHED_IN);

  GtkVBox* vbox2 = create_dialog_vbox(0, 4);
  gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(vbox2));

  {
    PrefPage preferencesPage(*this, GTK_WIDGET(vbox2));
    CreateGlobalFrame(&preferencesPage);
  }

  return create_simple_modal_dialog_window("Global Preferences", m_modal, GTK_WIDGET(frame));
}

// Search for game definitions

void GameDialog::ScanForGames()
{
	std::string gamesPath = GlobalRegistry().get(RKEY_APP_PATH) + "games/";
	
	globalOutputStream() << "Scanning for game description files: " << gamesPath.c_str() << '\n';

	// Invoke a GameFileLoader functor on every file in the games/ dir.
	Directory_forEach(gamesPath.c_str(), GameFileLoader(mGames, gamesPath.c_str()));
}

GameDescription* GameDialog::GameDescriptionForRegistryKey() {
  std::list<GameDescription *>::iterator iGame;
  int i=0;
  for(iGame=mGames.begin(); iGame!=mGames.end(); ++iGame,i++)
  {
    if (i == GlobalRegistry().getInt(RKEY_GAME_TYPE))
    {
      return (*iGame);
    }
  }
  return 0; // not found
}

void GameDialog::InitGlobalPrefPath() {
	g_Preferences._globalPrefPath = GlobalRegistry().get(RKEY_SETTINGS_PATH);
}

void GameDialog::Reset()
{
	if (g_Preferences._globalPrefPath.empty()) {
		InitGlobalPrefPath();
	}
	std::string globalPrefFile = g_Preferences._globalPrefPath + "global.pref";
	file_remove(globalPrefFile.c_str());
}

// Main initialisation routine for the game selection dialog. Scanning for
// .game files is called from here.

void GameDialog::initialise() {

	GlobalRegistry().addKeyObserver(this, RKEY_GAME_TYPE);

    InitGlobalPrefPath();
    LoadPrefs();

	_currentGameDescription = NULL;

    // Look for .game files and exit if no valid ones found
    ScanForGames();
    if (mGames.empty()) {
        Error("Didn't find any valid game file descriptions, aborting\n");
    }
    else {
	    // If the prompt is disabled, load the game type from the registry
	    _currentGameDescription = GameDescriptionForRegistryKey();
		
	    if (_currentGameDescription == NULL) {
			if (mGames.size() == 1) {
				// greebo: There is only one game type available, take it
				_currentGameDescription = *mGames.begin();
			}
			else {
				Create();
				DoGameDialog();
				
				_currentGameDescription = GameDescriptionForRegistryKey();
				ASSERT_NOTNULL(_currentGameDescription);
			}
	    }
    }
}

void GameDialog::setGameDescription(GameDescription* newGameDescription) {
	_currentGameDescription = newGameDescription;
}

GameDescription* GameDialog::getGameDescription() {
	return _currentGameDescription;
}

GameDialog& GameDialog::Instance() {
	static GameDialog _instance;
	return _instance;
}

} // namespace ui
