#pragma once

#include <map>

#include "imap.h"
#include "iautosaver.h"

#include <vector>
#include <sigc++/connection.h>
#include "os/fs.h"

namespace map
{

/**
 * greebo: The AutoMapSaver class lets itself being called in distinct intervals
 * and saves the map files either to snapshots or to a single yyyy.autosave.map file.
 */
class AutoMapSaver final : 
	public IAutomaticMapSaver
{
private:
	// TRUE, if the autosaver generates snapshots
	bool _snapshotsEnabled;

	std::size_t _changes;

	std::vector<sigc::connection> _signalConnections;

public:
	// Constructor
	AutoMapSaver();

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

	// Clears the _changes member variable that indicates how many changes have been made
	void clearChanges();

    // This performs is called to check if the map is valid/changed/should be saved
    // and calls the save routines accordingly.
    bool runAutosaveCheck() override;

    void performAutosave() override;

private:
	void constructPreferences();

	void registryKeyChanged();

	void onMapEvent(IMap::MapEvent ev);

	// Saves a snapshot of the currently active map (only named maps)
	void saveSnapshot();

	void collectExistingSnapshots(std::map<int, std::string>& existingSnapshots,
		const fs::path& snapshotPath, const std::string& mapName);

	void handleSnapshotSizeLimit(const std::map<int, std::string>& existingSnapshots,
		const fs::path& snapshotPath, const std::string& mapName);
};

} // namespace map
