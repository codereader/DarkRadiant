#include "AutoSaver.h"

#include "i18n.h"
#include <numeric>
#include <iostream>
#include "imapfilechangetracker.h"
#include "itextstream.h"
#include "iscenegraph.h"
#include "iradiant.h"
#include "iregistry.h"
#include "igame.h"
#include "ipreferencesystem.h"
#include "icommandsystem.h"

#include "registry/registry.h"

#include "os/file.h"
#include "os/path.h"
#include "os/dir.h"
#include "os/fs.h"
#include "gamelib.h"

#include <limits.h>
#include "string/string.h"
#include "string/convert.h"
#include "module/StaticModule.h"
#include "messages/NotificationMessage.h"
#include "messages/AutomaticMapSaveRequest.h"
#include "map/Map.h"

#include <fmt/format.h>

namespace map
{

namespace
{
	// Registry key names
	const char* GKEY_MAP_EXTENSION = "/mapFormat/fileExtension";

	std::string constructSnapshotName(const fs::path& snapshotPath, const std::string& mapName, int num)
	{
		std::string mapExt = game::current::getValue<std::string>(GKEY_MAP_EXTENSION);

		// Construct the base name without numbered extension
		std::string filename = (snapshotPath / mapName).replace_extension().string();

		// Now append the number and the map extension to the map name
		filename += ".";
		filename += string::to_string(num);
		filename += ".";
		filename += mapExt;

		return filename;
	}
}

AutoMapSaver::AutoMapSaver() :
	_snapshotsEnabled(false),
    _savedChangeCount(0)
{}

void AutoMapSaver::registryKeyChanged()
{
	_snapshotsEnabled = registry::getValue<bool>(RKEY_AUTOSAVE_SNAPSHOTS_ENABLED);
}

void AutoMapSaver::clearChanges()
{
    _savedChangeCount = 0;
}

void AutoMapSaver::saveSnapshot()
{
	// Original GtkRadiant comments:
	// we need to do the following
	// 1. make sure the snapshot directory exists (create it if it doesn't)
	// 2. find out what the lastest save is based on number
	// 3. inc that and save the map

	// Construct the fs::path class out of the full map path (throws on fail)
	fs::path fullPath = GlobalMapModule().getMapName();

    if (!fullPath.is_absolute())
    {
        fullPath = GlobalFileSystem().findFile(fullPath.string()) + fullPath.string();
    }

	// Assemble the absolute path to the snapshot folder
	fs::path snapshotPath = fullPath;
	snapshotPath.remove_filename();

    // If the snapshots folder in the registry is absolute, operator/= will use the absolute one
	snapshotPath /= GlobalRegistry().get(RKEY_AUTOSAVE_SNAPSHOTS_FOLDER);

	// Retrieve the mapname
	auto mapName = fullPath.filename().string();

	// Check if the folder exists and create it if necessary
	if (os::fileOrDirExists(snapshotPath.string()) || os::makeDirectory(snapshotPath.string()))
	{
        // Map existing snapshots (snapshot num => path)
        std::map<int, std::string> existingSnapshots;

		collectExistingSnapshots(existingSnapshots, snapshotPath, mapName);

		int highestNum = existingSnapshots.empty() ? 0 : existingSnapshots.rbegin()->first + 1;

		std::string filename = constructSnapshotName(snapshotPath, mapName, highestNum);

		rMessage() << "Autosaving snapshot to " << filename << std::endl;

		// Dump to map to the next available filename
        GlobalCommandSystem().executeCommand("SaveAutomaticBackup", filename);

		handleSnapshotSizeLimit(existingSnapshots, snapshotPath, mapName);
	}
	else
	{
		rError() << "Snapshot save failed, unable to create directory " << snapshotPath << std::endl;
	}
}

void AutoMapSaver::handleSnapshotSizeLimit(const std::map<int, std::string>& existingSnapshots,
	const fs::path& snapshotPath, const std::string& mapName)
{
	std::size_t maxSnapshotFolderSize =
		registry::getValue<std::size_t>(RKEY_AUTOSAVE_MAX_SNAPSHOT_FOLDER_SIZE);

	// Sanity check in case there is something weird going on in the registry
	if (maxSnapshotFolderSize == 0)
	{
		maxSnapshotFolderSize = 100;
	}

	// Sum up the total folder size
	std::size_t folderSize = 0;

	for (const std::pair<int, std::string>& pair : existingSnapshots)
	{
		folderSize += os::getFileSize(pair.second);
	}

	std::size_t maxSize = maxSnapshotFolderSize * 1024 * 1024;

	// The key containing the previously calculated size
	std::string mapKey = RKEY_AUTOSAVE_SNAPSHOT_FOLDER_SIZE_HISTORY;
	mapKey += "/map[@name='" + mapName + "']";

	// Display a warning, if the folder size exceeds the limit
	if (folderSize > maxSize)
	{
		std::size_t prevSize = string::convert<std::size_t>(GlobalRegistry().getAttribute(mapKey, "size"), 0);

		// Save the size for the next time we hit it
		GlobalRegistry().deleteXPath(mapKey);

		// Create a new key and store the size
		GlobalRegistry().createKeyWithName(RKEY_AUTOSAVE_SNAPSHOT_FOLDER_SIZE_HISTORY, "map", mapName);
		GlobalRegistry().setAttribute(mapKey, "size", string::to_string(folderSize));

		// Now should we display a message?
		if (prevSize > maxSize)
		{
			rMessage() << "User has already been notified about the snapshot size exceeding limits." << std::endl;
			return;
		}

		rMessage() << "AutoSaver: The snapshot files in " << snapshotPath <<
			" take up more than " << maxSnapshotFolderSize << " MB. You might consider cleaning it up." << std::endl;

		// Notify the user
		radiant::NotificationMessage::SendWarning(
			fmt::format(_("The snapshots saved for this map are exceeding the configured size limit."
				"\nConsider cleaning up the folder {0}"), snapshotPath.string()));
	}
	else
	{
		// Folder size is within limits (again), delete the size info from the registry
		GlobalRegistry().deleteXPath(mapKey);
	}
}

void AutoMapSaver::collectExistingSnapshots(std::map<int, std::string>& existingSnapshots,
	const fs::path& snapshotPath, const std::string& mapName)
{
	for (int num = 0; num < INT_MAX; num++)
	{
		// Construct the base name without numbered extension
		std::string filename = constructSnapshotName(snapshotPath, mapName, num);

		if (!os::fileOrDirExists(filename))
		{
			return; // We've found an unused filename, break the loop
		}

		existingSnapshots.insert(std::make_pair(num, filename));
	}
}

bool AutoMapSaver::runAutosaveCheck()
{
    // Check, if changes have been made since the last autosave
    if (!GlobalSceneGraph().root() || _savedChangeCount == GlobalSceneGraph().root()->getUndoChangeTracker().getCurrentChangeCount())
    {
        return false;
    }

    AutomaticMapSaveRequest request;
    GlobalRadiantCore().getMessageBus().sendMessage(request);

    if (request.isDenied())
    {
        rMessage() << "Auto save skipped: " << request.getReason() << std::endl;
        return false;
    }

    return true;
}

void AutoMapSaver::performAutosave()
{
    // Remember the change tracking counter
    _savedChangeCount = GlobalSceneGraph().root()->getUndoChangeTracker().getCurrentChangeCount();

    // only snapshot if not working on an unnamed map
    if (_snapshotsEnabled && !GlobalMapModule().isUnnamed())
    {
        try
        {
            saveSnapshot();
        }
        catch (fs::filesystem_error& f)
        {
            rError() << "AutoSaver::saveSnapshot: " << f.what() << std::endl;
        }
    }
    else
    {
        if (GlobalMapModule().isUnnamed())
        {
            // Get the maps path (within the mod path)
            auto autoSaveFilename = GlobalGameManager().getMapPath();

            // Try to create the map folder, in case there doesn't exist one
            os::makeDirectory(autoSaveFilename);

            // Append the "autosave.map" to the filename
            autoSaveFilename += "autosave.";
            autoSaveFilename += game::current::getValue<std::string>(GKEY_MAP_EXTENSION);

            rMessage() << "Autosaving unnamed map to " << autoSaveFilename << std::endl;

            // Invoke the save call
            GlobalCommandSystem().executeCommand("SaveAutomaticBackup", autoSaveFilename);
        }
        else
        {
            // Construct the new filename (e.g. "test_autosave.map")
            auto filename = GlobalMapModule().getMapName();
            auto extension = os::getExtension(filename);

            // Cut off the extension
            filename = filename.substr(0, filename.rfind('.'));
            filename += "_autosave";
            filename += "." + extension;

            rMessage() << "Autosaving map to " << filename << std::endl;

            // Invoke the save call
            GlobalCommandSystem().executeCommand("SaveAutomaticBackup", filename);
        }
    }
}

void AutoMapSaver::constructPreferences()
{
	// Add a page to the given group
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Autosave"));

	page.appendCheckBox(_("Save Snapshots"), RKEY_AUTOSAVE_SNAPSHOTS_ENABLED);
	page.appendEntry(_("Snapshot Folder (absolute, or relative to Map Folder)"), RKEY_AUTOSAVE_SNAPSHOTS_FOLDER);
	page.appendEntry(_("Max total Snapshot size per Map (MB)"), RKEY_AUTOSAVE_MAX_SNAPSHOT_FOLDER_SIZE);
}

void AutoMapSaver::onMapEvent(IMap::MapEvent ev)
{
	// We reset our change count regardless of whether a map
	// is loaded or unloaded
	switch (ev)
	{
	case IMap::MapLoading:
	case IMap::MapLoaded:
	case IMap::MapUnloading:
	case IMap::MapUnloaded:
		clearChanges();
		break;
    default:
        break;
	};
}

const std::string& AutoMapSaver::getName() const
{
	static std::string _name(MODULE_AUTOSAVER);
	return _name;
}

const StringSet& AutoMapSaver::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_MAP);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_XMLREGISTRY);
	}

	return _dependencies;
}

void AutoMapSaver::initialiseModule(const IApplicationContext& ctx)
{
	_signalConnections.push_back(GlobalRegistry().signalForKey(RKEY_AUTOSAVE_SNAPSHOTS_ENABLED).connect(
		sigc::mem_fun(this, &AutoMapSaver::registryKeyChanged)
	));

	// Get notified when the map is loaded afresh
	_signalConnections.push_back(GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(*this, &AutoMapSaver::onMapEvent)
	));

	// Refresh all values from the registry right now (this might also start the timer)
	registryKeyChanged();

    // Add the autosave options after all the modules are done. A cheap solution to let
    // the options appear below the Enabled / Interval setting added by the UI
    module::GlobalModuleRegistry().signal_allModulesInitialised().connect(
        sigc::mem_fun(*this, &AutoMapSaver::constructPreferences)
    );
}

void AutoMapSaver::shutdownModule()
{
	// Unsubscribe from all connections
	for (sigc::connection& connection : _signalConnections)
	{
		connection.disconnect();
	}

	_signalConnections.clear();
}

module::StaticModuleRegistration<AutoMapSaver> staticAutoSaverModule;

} // namespace map
