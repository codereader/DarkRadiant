#include "AutoSaver.h"

#include "i18n.h"
#include <numeric>
#include <iostream>
#include "mapfile.h"
#include "itextstream.h"
#include "iscenegraph.h"
#include "iradiant.h"
#include "imainframe.h"
#include "ipreferencesystem.h"

#include "registry/registry.h"

#include "os/file.h"
#include "os/path.h"
#include "os/dir.h"
#include "os/fs.h"
#include "gamelib.h"

#include <limits.h>
#include "string/string.h"
#include "string/convert.h"
#include "map/Map.h"
#include "modulesystem/ApplicationContextImpl.h"
#include "modulesystem/StaticModule.h"
#include "wxutil/dialog/MessageBox.h"

#include <fmt/format.h>
#include <wx/frame.h>

namespace map 
{

namespace 
{
	// Registry key names
	const char* RKEY_AUTOSAVE_ENABLED = "user/ui/map/autoSaveEnabled";
	const char* RKEY_AUTOSAVE_INTERVAL = "user/ui/map/autoSaveInterval";
	const char* RKEY_AUTOSAVE_SNAPSHOTS_ENABLED = "user/ui/map/autoSaveSnapshots";
	const char* RKEY_AUTOSAVE_SNAPSHOTS_FOLDER = "user/ui/map/snapshotFolder";
	const char* RKEY_AUTOSAVE_MAX_SNAPSHOT_FOLDER_SIZE = "user/ui/map/maxSnapshotFolderSize";
	const char* RKEY_AUTOSAVE_SNAPSHOT_FOLDER_SIZE_HISTORY = "user/ui/map/snapshotFolderSizeHistory";
	const char* GKEY_MAP_EXTENSION = "/mapFormat/fileExtension";

	std::string constructSnapshotName(const fs::path& snapshotPath, const std::string& mapName, int num)
	{
		std::string mapExt = game::current::getValue<std::string>(GKEY_MAP_EXTENSION);

		// Construct the base name without numbered extension
		std::string filename = (snapshotPath / mapName).string();

		// Now append the number and the map extension to the map name
		filename += ".";
		filename += string::to_string(num);
		filename += ".";
		filename += mapExt;

		return filename;
	}
}

AutoMapSaver::AutoMapSaver() :
	_enabled(false),
	_snapshotsEnabled(false),
	_interval(5*60),
	_changes(0)
{}

AutoMapSaver::~AutoMapSaver() 
{
	assert(!_timer);
}

void AutoMapSaver::registryKeyChanged() 
{
	// Stop the current timer
	stopTimer();

	_enabled = registry::getValue<bool>(RKEY_AUTOSAVE_ENABLED);
	_snapshotsEnabled = registry::getValue<bool>(RKEY_AUTOSAVE_SNAPSHOTS_ENABLED);
	_interval = registry::getValue<int>(RKEY_AUTOSAVE_INTERVAL) * 60;
	
	// Start the timer with the new interval
	if (_enabled)
	{
		startTimer();
	}
}

void AutoMapSaver::clearChanges()
{
	_changes = 0;
}

void AutoMapSaver::startTimer()
{
	if (!_timer) return;

	_timer->Start(static_cast<int>(_interval * 1000));
}

void AutoMapSaver::stopTimer()
{
	if (!_timer) return;

	_timer->Stop();
}

void AutoMapSaver::saveSnapshot() 
{
	// Original GtkRadiant comments:
	// we need to do the following
	// 1. make sure the snapshot directory exists (create it if it doesn't)
	// 2. find out what the lastest save is based on number
	// 3. inc that and save the map

	// Construct the fs::path class out of the full map path (throws on fail)
	fs::path fullPath = GlobalMap().getMapName();

	// Append the the snapshot folder to the path
	fs::path snapshotPath = fullPath;
	snapshotPath.remove_filename();
	snapshotPath /= GlobalRegistry().get(RKEY_AUTOSAVE_SNAPSHOTS_FOLDER);

	// Retrieve the mapname
	std::string mapName = fullPath.filename().string();

	// Map existing snapshots (snapshot num => path)
	std::map<int, std::string> existingSnapshots;

	// Check if the folder exists and create it if necessary
	if (os::fileOrDirExists(snapshotPath.string()) || os::makeDirectory(snapshotPath.string()))
	{
		collectExistingSnapshots(existingSnapshots, snapshotPath, mapName);

		int highestNum = existingSnapshots.empty() ? 0 : existingSnapshots.rbegin()->first + 1;

		std::string filename = constructSnapshotName(snapshotPath, mapName, highestNum);

		rMessage() << "Autosaving snapshot to " << filename << std::endl;

		// Dump to map to the next available filename
		GlobalMap().saveDirect(filename);

		handleSnapshotSizeLimit(existingSnapshots, snapshotPath, mapName);
	}
	else 
	{
		rError() << "Snapshot save failed.. unable to create directory";
		rError() << snapshotPath << std::endl;
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
		wxutil::Messagebox::Show(_("Snapshot Folder Size Warning"),
			fmt::format(_("The snapshots saved for this map are exceeding the configured size limit."
				"\nConsider cleaning up the folder {0}"), snapshotPath.string()), ui::IDialog::MessageType::MESSAGE_WARNING);
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

void AutoMapSaver::checkSave()
{
	if (!GlobalMainFrame().screenUpdatesEnabled())
	{
		return;
	}

	// greebo: Check if we have a valid main window to grab the pointer
	wxFrame* mainWindow = GlobalMainFrame().getWxTopLevelWindow();

	if (mainWindow == NULL || !mainWindow->IsActive())
	{
		rMessage() << "AutoSaver: Main window not present or not shown on screen, " <<
			 "will wait for another period." << std::endl;
		return;
	}

	// Check, if changes have been made since the last autosave
    if (!GlobalSceneGraph().root() ||
		_changes == GlobalSceneGraph().root()->getUndoChangeTracker().changes())
	{
		return;
	}

	// Check if the user is currently pressing a mouse button
	// Don't start the save if the user is holding a mouse button
	if (wxGetMouseState().ButtonIsDown(wxMOUSE_BTN_ANY)) 
	{
		return;
	}

    _changes = GlobalSceneGraph().root()->getUndoChangeTracker().changes();

	// Stop the timer before saving
	stopTimer();

	if (_enabled)
	{
		// only snapshot if not working on an unnamed map
		if (_snapshotsEnabled && !GlobalMap().isUnnamed())
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
			if (GlobalMap().isUnnamed())
			{
				// Get the maps path (within the mod path)
				std::string autoSaveFilename = GlobalRegistry().get(RKEY_MAP_PATH);

				// Try to create the map folder, in case there doesn't exist one
				os::makeDirectory(autoSaveFilename);

				// Append the "autosave.map" to the filename
				autoSaveFilename += "autosave.";
				autoSaveFilename += game::current::getValue<std::string>(GKEY_MAP_EXTENSION);

				rMessage() << "Autosaving unnamed map to " << autoSaveFilename << std::endl;

				// Invoke the save call
				GlobalMap().saveDirect(autoSaveFilename);
			}
			else
			{
				// Construct the new filename (e.g. "test_autosave.map")
				std::string filename = GlobalMap().getMapName();
				std::string extension = os::getExtension(filename);

				// Cut off the extension
				filename = filename.substr(0, filename.rfind('.'));
				filename += "_autosave";
				filename += "." + extension;

				rMessage() << "Autosaving map to " << filename << std::endl;

				// Invoke the save call
				GlobalMap().saveDirect(filename);
			}
		}
	}
	else
	{
		rMessage() << "Autosave skipped..." << std::endl;
	}

	// Re-start the timer after saving has finished
	startTimer();
}

void AutoMapSaver::constructPreferences()
{
	// Add a page to the given group
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Autosave"));

	// Add the checkboxes and connect them with the registry key and the according observer
	page.appendCheckBox(_("Enable Autosave"), RKEY_AUTOSAVE_ENABLED);
	page.appendSlider(_("Autosave Interval (in minutes)"), RKEY_AUTOSAVE_INTERVAL, 1, 61, 1, 1);

	page.appendCheckBox(_("Save Snapshots"), RKEY_AUTOSAVE_SNAPSHOTS_ENABLED);
	page.appendEntry(_("Snapshot folder (relative to map folder)"), RKEY_AUTOSAVE_SNAPSHOTS_FOLDER);
	page.appendEntry(_("Max total Snapshot size per map (MB)"), RKEY_AUTOSAVE_MAX_SNAPSHOT_FOLDER_SIZE);
}

void AutoMapSaver::onIntervalReached(wxTimerEvent& ev)
{
	checkSave();
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
	static std::string _name("AutomaticMapSaver");
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
		_dependencies.insert(MODULE_MAINFRAME);
		_dependencies.insert(MODULE_RADIANT);
	}

	return _dependencies;
}

void AutoMapSaver::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	_timer.reset(new wxTimer(this));

	constructPreferences();

	Connect(wxEVT_TIMER, wxTimerEventHandler(AutoMapSaver::onIntervalReached), NULL, this);

	_signalConnections.push_back(GlobalRegistry().signalForKey(RKEY_AUTOSAVE_INTERVAL).connect(
		sigc::mem_fun(this, &AutoMapSaver::registryKeyChanged)
	));
	_signalConnections.push_back(GlobalRegistry().signalForKey(RKEY_AUTOSAVE_SNAPSHOTS_ENABLED).connect(
		sigc::mem_fun(this, &AutoMapSaver::registryKeyChanged)
	));
	_signalConnections.push_back(GlobalRegistry().signalForKey(RKEY_AUTOSAVE_ENABLED).connect(
		sigc::mem_fun(this, &AutoMapSaver::registryKeyChanged)
	));

	// Get notified when the map is loaded afresh
	_signalConnections.push_back(GlobalMap().signal_mapEvent().connect(
		sigc::mem_fun(*this, &AutoMapSaver::onMapEvent)
	));

	// Refresh all values from the registry right now (this might also start the timer)
	registryKeyChanged();
}

void AutoMapSaver::shutdownModule()
{
	// Unsubscribe from all connections
	for (sigc::connection& connection : _signalConnections)
	{
		connection.disconnect();
	}

	_signalConnections.clear();

	_enabled = false;
	stopTimer();

	// Destroy the timer
	_timer.reset();
}

module::StaticModule<AutoMapSaver> staticAutoSaverModule;

AutoMapSaver& AutoSaver()
{
	return *staticAutoSaverModule.getModule();
}

} // namespace map
