#pragma once

#include <map>

#include "iregistry.h"
#include "imodule.h"
#include "imap.h"

#include <vector>
#include <sigc++/connection.h>
#include <wx/timer.h>
#include <wx/sharedptr.h>
#include "os/fs.h"

/* greebo: The AutoMapSaver class lets itself being called in distinct intervals
 * and saves the map files either to snapshots or to a single yyyy.autosave.map file.
 */

namespace map
{

class AutoMapSaver : 
	public wxEvtHandler,
	public RegisterableModule
{
	// TRUE, if autosaving is enabled
	bool _enabled;

	// TRUE, if the autosaver generates snapshots
	bool _snapshotsEnabled;

	// The autosave interval stored in seconds
	unsigned long _interval;

	// The timer object that triggers the callback
	wxSharedPtr<wxTimer> _timer;

	std::size_t _changes;

	std::vector<sigc::connection> _signalConnections;

public:
	// Constructor
	AutoMapSaver();

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

	~AutoMapSaver();

	// Starts/stops the autosave "countdown"
	void startTimer();
	void stopTimer();

	// Clears the _changes member variable that indicates how many changes have been made
	void clearChanges();

private:
	// Adds the elements to the according preference page
	void constructPreferences();

	void registryKeyChanged();

	void onMapEvent(IMap::MapEvent ev);

	// This performs is called to check if the map is valid/changed/should be saved
	// and calls the save routines accordingly.
	void checkSave();

	// Saves a snapshot of the currently active map (only named maps)
	void saveSnapshot();

	// This gets called when the interval time is over
	void onIntervalReached(wxTimerEvent& ev);

	void collectExistingSnapshots(std::map<int, std::string>& existingSnapshots,
		const fs::path& snapshotPath, const std::string& mapName);

	void handleSnapshotSizeLimit(const std::map<int, std::string>& existingSnapshots,
		const fs::path& snapshotPath, const std::string& mapName);

}; // class AutoMapSaver

// The accessor function for the static AutoSaver instance
AutoMapSaver& AutoSaver();

} // namespace map
