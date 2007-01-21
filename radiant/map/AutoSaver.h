#ifndef AUTOSAVER_H_
#define AUTOSAVER_H_

#include <string>
#include "iregistry.h"
#include "preferencesystem.h"

#include "gtkutil/Timer.h"
#include <boost/filesystem/path.hpp>

/* greebo: The AutoMapSaver class lets itself being called in distinct intervals
 * and saves the map files either to snapshots or to a single yyyy.autosave.map file.  
 */

namespace map {
	
	namespace {
		const std::string RKEY_AUTOSAVE_ENABLED = "user/ui/map/autoSaveEnabled";
		const std::string RKEY_AUTOSAVE_INTERVAL = "user/ui/map/autoSaveInterval";
		const std::string RKEY_AUTOSAVE_SNAPSHOTS_ENABLED = "user/ui/map/autoSaveSnapshots";
		const std::string RKEY_AUTOSAVE_SNAPSHOTS_FOLDER = "user/ui/map/snapshotFolder";
		const std::string RKEY_AUTOSAVE_MAX_SNAPSHOT_FOLDER_SIZE = "user/ui/map/maxSnapshotFolderSize";
		const std::string RKEY_MAP_FOLDER = "game/mapFormat/mapFolder";
		const std::string RKEY_MAP_EXTENSION = "game/mapFormat/fileExtension";
	}
	
class AutoMapSaver :
	public RegistryKeyObserver,
	public PreferenceConstructor
{
	typedef boost::filesystem::path Path;
	
	// TRUE, if autosaving is enabled
	bool _enabled;
	
	// TRUE, if the autosaver generates snapshots
	bool _snapshotsEnabled;
	
	// The autosave interval stored in seconds
	unsigned long _interval;
	
	// The timer object that triggers the callback
	gtkutil::Timer _timer;
	
	std::size_t _changes;
	
public:
	// Constructor
	AutoMapSaver();
	
	// Initialises the preferences and the registrykeyobserver
	void init();
	
	~AutoMapSaver();
	
	// Starts/stops the autosave "countdown"
	void startTimer();
	void stopTimer();
	
	// Clears the _changes member variable that indicates how many changes have been made
	void clearChanges();
	
	// The RegistryKeyObserver implementation - gets called upon key change
	void keyChanged();
	
	// PreferenceConstructor implementation, adds the elements to the according preference page
	void constructPreferencePage(PreferenceGroup& group);

private:
	// This performs is called to check if the map is valid/changed/should be saved
	// and calls the save routines accordingly.
	void checkSave();
	
	// Saves a snapshot of the currently active map (only named maps)
	void saveSnapshot();

	// This gets called by GTK when the interval time is over
	static gboolean onIntervalReached(gpointer data);

}; // class AutoMapSaver
	
// The accessor function for the static AutoSaver instance
AutoMapSaver& AutoSaver();

} // namespace map

#endif /*AUTOSAVER_H_*/
