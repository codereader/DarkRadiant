#pragma once

#include "imodule.h"

namespace map
{

/**
 * Public interface to to automatic map save algorithms.
 * 
 * DarkRadiant can be configured to create map backups on demand,
 * which can be a simple copy of the current map that is overwritten every time,
 * or it can create sequential snapshots in a separate folder including size limitations.
 * 
 * The configuration of the save behaviour is done through registry keys.
 */
class IAutomaticMapSaver :
    public RegisterableModule
{
public:
    virtual ~IAutomaticMapSaver() {}

    // Run the checks to see if the map needs to be automatically saved.
    // If the auto saver is not enabled, nothing happens.
    // Returns true if the map is ready to be auto-saved, false otherwise.
    virtual bool runAutosaveCheck() = 0;

    // Perform an automatic save, unconditionally. This will run the save algorithms
    // for the currently loaded map, regardless whether it is due for a save or not.
    // Call the "runAutosaveCheck" method to see if an autosave is overdue.
    virtual void performAutosave() = 0;
};

constexpr const char* const RKEY_AUTOSAVE_SNAPSHOTS_ENABLED = "user/ui/map/autoSaveSnapshots";
constexpr const char* const RKEY_AUTOSAVE_SNAPSHOTS_FOLDER = "user/ui/map/snapshotFolder";
constexpr const char* const RKEY_AUTOSAVE_MAX_SNAPSHOT_FOLDER_SIZE = "user/ui/map/maxSnapshotFolderSize";
constexpr const char* const RKEY_AUTOSAVE_SNAPSHOT_FOLDER_SIZE_HISTORY = "user/ui/map/snapshotFolderSizeHistory";

}

constexpr const char* const MODULE_AUTOSAVER("AutomaticMapSaver");

// Application-wide Accessor to the global map auto saver
inline map::IAutomaticMapSaver& GlobalAutoSaver()
{
    static module::InstanceReference<map::IAutomaticMapSaver> _reference(MODULE_AUTOSAVER);
    return _reference;
}
