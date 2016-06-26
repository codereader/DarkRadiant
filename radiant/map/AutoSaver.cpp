#include "AutoSaver.h"

#include "i18n.h"
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
#include "map/Map.h"
#include "modulesystem/ApplicationContextImpl.h"
#include "modulesystem/StaticModule.h"

#include <wx/frame.h>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>

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
	const char* GKEY_MAP_EXTENSION = "/mapFormat/fileExtension";

	// Filesystem path typedef
	typedef boost::filesystem::path Path;
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
	assert(_timer);
	if (!_timer) return;

	_timer->Start(_interval * 1000);
}

void AutoMapSaver::stopTimer()
{
	assert(_timer);
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

	unsigned int maxSnapshotFolderSize =
		registry::getValue<int>(RKEY_AUTOSAVE_MAX_SNAPSHOT_FOLDER_SIZE);

	// Sanity check in case there is something weird going on in the registry
	if (maxSnapshotFolderSize == 0)
	{
		maxSnapshotFolderSize = 100;
	}

	// Construct the boost::path class out of the full map path (throws on fail)
	Path fullPath = GlobalMap().getMapName();

	// Append the the snapshot folder to the path
	Path snapshotPath = fullPath;
	snapshotPath.remove_filename();
	snapshotPath /= GlobalRegistry().get(RKEY_AUTOSAVE_SNAPSHOTS_FOLDER);

	// Retrieve the mapname
	std::string mapName = os::filename_from_path(fullPath.filename());

	// Check if the folder exists and create it if necessary
	if (boost::filesystem::exists(snapshotPath) || os::makeDirectory(snapshotPath.string()))
	{
		// Reset the size counter of the snapshots folder
		std::size_t folderSize = 0;

		// This holds the target path of the snapshot
		std::string filename;

		for (int nCount = 0; nCount < INT_MAX; nCount++) 
		{
			// Construct the base name without numbered extension
			filename = (snapshotPath / mapName).string();

			// Now append the number and the map extension to the map name
			filename += ".";
			filename += string::to_string(nCount);
			filename += ".";
			filename += game::current::getValue<std::string>(GKEY_MAP_EXTENSION);

			if (os::fileOrDirExists(filename)) 
			{
				// Add to the folder size
				folderSize += file_size(filename.c_str());
			}
			else 
			{
				// We've found an unused filename, break the loop
				break;
			}
		}

		rMessage() << "Autosaving snapshot to " << filename << std::endl;

		// Dump to map to the next available filename
		GlobalMap().saveDirect(filename);

		// Display a warning, if the folder size exceeds the limit
		if (folderSize > maxSnapshotFolderSize*1024*1024)
		{
			rMessage() << "AutoSaver: The snapshot files in " << snapshotPath;
			rMessage() << " total more than " << maxSnapshotFolderSize;
			rMessage() << " MB. You might consider cleaning up." << std::endl;
		}
	}
	else 
	{
		rError() << "Snapshot save failed.. unable to create directory";
		rError() << snapshotPath << std::endl;
	}
}

void AutoMapSaver::checkSave()
{
	// Check if we have a proper map
	if (!GlobalMap().isValid() || !GlobalMainFrame().screenUpdatesEnabled())
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
    if (_changes == GlobalSceneGraph().root()->getUndoChangeTracker().changes())
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
			catch (boost::filesystem::filesystem_error& f) 
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
	page.appendEntry(_("Max Snapshot Folder size (MB)"), RKEY_AUTOSAVE_MAX_SNAPSHOT_FOLDER_SIZE);
}

void AutoMapSaver::onIntervalReached(wxTimerEvent& ev)
{
	checkSave();
}

void AutoMapSaver::onMapEvent(IRadiant::MapEvent ev)
{
	// We reset our change count regardless of whether a map
	// is loaded or unloaded
	switch (ev)
	{
	case IRadiant::MapLoading:
	case IRadiant::MapLoaded:
	case IRadiant::MapUnloading:
	case IRadiant::MapUnloaded:
		clearChanges();
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
	_signalConnections.push_back(GlobalRadiant().signal_mapEvent().connect(
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
