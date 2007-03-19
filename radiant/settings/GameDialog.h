#ifndef GAMEDIALOG_H_
#define GAMEDIALOG_H_

#include "iregistry.h"
#include <string>
#include "dialog.h"
#include "stream/stringstream.h"

typedef struct _GtkWindow GtkWindow; 
class PrefPage;
class GameDescription;

/*!
standalone dialog for games selection, and more generally global settings
*/
class GameDialog : 
	public Dialog,
	public RegistryKeyObserver
{
public:

	// Gets notified upon game type changes
	void keyChanged();

  /*!
  prompt which game to load on startup
  */
  bool m_bGamePrompt;
  
  /*!
  the list of game descriptions we scanned from the game/ dir
  */
  std::list<GameDescription*> mGames;

  GameDialog();
  virtual ~GameDialog(); 

  void AddPacksURL(StringOutputStream &s);  
    
  /*!
  intialize the game dialog, called at CPrefsDlg::Init
  will scan for games, load prefs, and do game selection dialog if needed
  */
  void Init();

  /*!
  reset the global settings by removing the file
  */
  void Reset();

  /*!
  run the dialog UI for the list of games 
  */
  void DoGameDialog();

  /*!
  Dialog API
  this is only called when the dialog is built at startup for main engine select
  */
  GtkWindow* BuildDialog();

  void GameFileImport(int value);
  void GameFileExport(const IntImportCallback& importCallback) const;

  /*!
  construction of the dialog frame
  this is the part to be re-used in prefs dialog
  for the standalone dialog, we include this in a modal box
  for prefs, we hook the frame in the main notebook
  build the frame on-demand (only once)
  */
  void CreateGlobalFrame(PrefPage* page);

  /*!
  global preferences subsystem
  XML-based this time, hopefully this will generalize to other prefs
  LoadPrefs has hardcoded defaults
  NOTE: it may not be strictly 'GameDialog' to put the global prefs here
    could have named the class differently I guess
  */
  /*@{*/
  void LoadPrefs(); ///< load from file into variables
  void SavePrefs(); ///< save pref variables to file
  /*@}*/

private:
  /*!
  scan for .game files, load them
  */
  void ScanForGames();

  /*!
  inits g_Preferences.m_global_rc_path
  */
  void InitGlobalPrefPath();

  /*!
  uses m_nComboItem to find the right mGames
  */
  GameDescription *GameDescriptionForRegistryKey();
};

#endif /*GAMEDIALOG_H_*/
