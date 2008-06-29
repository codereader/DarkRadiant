#include "AutoSaver.h"

#include <iostream>
#include "mapfile.h"
#include "iscenegraph.h"
#include "iradiant.h"
#include "ipreferencesystem.h"

#include "gdk/gdkwindow.h"

#include "os/file.h"
#include "os/dir.h"
#include "string/string.h"
#include "map/Map.h"
#include "mainframe.h"
#include "modulesystem/ApplicationContextImpl.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>

namespace map {

namespace {
	
	/* Registry key names */
	const char* RKEY_AUTOSAVE_ENABLED = "user/ui/map/autoSaveEnabled";
	const char* RKEY_AUTOSAVE_INTERVAL = "user/ui/map/autoSaveInterval";
	const char* RKEY_AUTOSAVE_SNAPSHOTS_ENABLED = "user/ui/map/autoSaveSnapshots";
	const char* RKEY_AUTOSAVE_SNAPSHOTS_FOLDER = "user/ui/map/snapshotFolder";
	const char* RKEY_AUTOSAVE_MAX_SNAPSHOT_FOLDER_SIZE = "user/ui/map/maxSnapshotFolderSize";
	const char* RKEY_MAP_FOLDER = "game/mapFormat/mapFolder";
	const char* RKEY_MAP_EXTENSION = "game/mapFormat/fileExtension";
	
	// Filesystem path typedef
	typedef boost::filesystem::path Path;
}
	
	
AutoMapSaver::AutoMapSaver() :
	_enabled(GlobalRegistry().get(RKEY_AUTOSAVE_ENABLED) == "1"),
	_snapshotsEnabled(GlobalRegistry().get(RKEY_AUTOSAVE_SNAPSHOTS_ENABLED) == "1"),
	_interval(GlobalRegistry().getInt(RKEY_AUTOSAVE_INTERVAL) * 60),
	_timer(_interval*1000, onIntervalReached, this),
	_changes(0)
{
	GlobalRegistry().addKeyObserver(this, RKEY_AUTOSAVE_INTERVAL);
	GlobalRegistry().addKeyObserver(this, RKEY_AUTOSAVE_SNAPSHOTS_ENABLED);
	GlobalRegistry().addKeyObserver(this, RKEY_AUTOSAVE_ENABLED);
}

AutoMapSaver::~AutoMapSaver() {
	stopTimer();
}

void AutoMapSaver::keyChanged(const std::string& key, const std::string& val) {
	// Stop the current timer
	stopTimer();
	
	_enabled = (GlobalRegistry().get(RKEY_AUTOSAVE_ENABLED) == "1");
	_snapshotsEnabled = (GlobalRegistry().get(RKEY_AUTOSAVE_SNAPSHOTS_ENABLED) == "1");
	_interval = GlobalRegistry().getInt(RKEY_AUTOSAVE_INTERVAL) * 60;

	// Update the internal timer
	_timer.setTimeout(_interval * 1000);
	
	// Start the timer with the new interval
	if (_enabled) {
		startTimer();
	}
}

void AutoMapSaver::init() {
	constructPreferences();
}

void AutoMapSaver::clearChanges() {
	_changes = 0;
}

void AutoMapSaver::startTimer() {
	_timer.enable();
}

void AutoMapSaver::stopTimer() {
	_timer.disable();
}

void AutoMapSaver::saveSnapshot() {
	// Original GtkRadiant comments:
	// we need to do the following
	// 1. make sure the snapshot directory exists (create it if it doesn't)
	// 2. find out what the lastest save is based on number
	// 3. inc that and save the map
	
	unsigned int maxSnapshotFolderSize = 
		GlobalRegistry().getInt(RKEY_AUTOSAVE_MAX_SNAPSHOT_FOLDER_SIZE);
	
	// Sanity check in case there is something weird going on in the registry
	if (maxSnapshotFolderSize == 0) {
		maxSnapshotFolderSize = 100;
	}
	
	// Construct the boost::path class out of the full map path (throws on fail)
	Path fullPath = Path(GlobalMap().getMapName(), boost::filesystem::native);
	
	// Append the the snapshot folder to the path
	std::string snapshotPath = fullPath.branch_path().string() + "/";
	snapshotPath += GlobalRegistry().get(RKEY_AUTOSAVE_SNAPSHOTS_FOLDER);
	
	// Retrieve the mapname
	std::string mapName = fullPath.leaf();
	
	// Check if the folder exists and create it if necessary
	if (file_exists(snapshotPath.c_str()) || os::makeDirectory(snapshotPath)) {
		
		// Reset the size counter of the snapshots folder 
		std::size_t folderSize = 0;
		
		// This holds the target path of the snapshot
		std::string filename;
		
		for (int nCount = 0; nCount < 5000; nCount++) {
			
			// Construct the base name without numbered extension
			filename = snapshotPath + mapName;
			
			std::cout << filename.c_str() << "\n";
			// Now append the number and the map extension to the map name
			filename += ".";
			filename += intToStr(nCount);
			filename += ".";
			filename += GlobalRegistry().get(RKEY_MAP_EXTENSION);
			
			if (file_exists(filename.c_str())) {
				// Add to the folder size
				folderSize += file_size(filename.c_str());
			}
			else {
				// We've found an unused filename, break the loop
				break;
			}
		}

		globalOutputStream() << "Autosaving snapshot to " << filename.c_str() << "\n"; 
		
		// Dump to map to the next available filename
		GlobalMap().saveDirect(filename);

		// Display a warning, if the folder size exceeds the limit 
		if (folderSize > maxSnapshotFolderSize*1024*1024) {
			globalOutputStream() << "AutoSaver: The snapshot files in " << snapshotPath.c_str();
			globalOutputStream() << " total more than " << maxSnapshotFolderSize;
			globalOutputStream() << " MB. You might consider cleaning up.\n";
		}
	}
	else {
		globalErrorStream() << "Snapshot save failed.. unable to create directory";
		globalErrorStream() << snapshotPath.c_str() << "\n";
	}
}

void AutoMapSaver::checkSave() {
	// Check if we have a proper map
	if (!GlobalMap().isValid() || !ScreenUpdates_Enabled()) {
		return;
	}

	// greebo: Check if we have a valid main window to grab the pointer
	GtkWindow* mainWindow = GlobalRadiant().getMainWindow();
	if (mainWindow == NULL) {
		return;
	}

	// Get the GdkWindow from the widget
	GdkWindow* mainGDKWindow = GTK_WIDGET(mainWindow)->window;
	if (!GDK_IS_WINDOW(mainGDKWindow)) {
		// Window might not be "shown" yet
		return;
	}

	// Check if the user is currently pressing a mouse button
	GdkModifierType mask;
	gdk_window_get_pointer(mainGDKWindow, 0, 0, &mask);
	
	const unsigned int anyButton = (
		GDK_BUTTON1_MASK | GDK_BUTTON2_MASK |
		GDK_BUTTON3_MASK | GDK_BUTTON4_MASK |
		GDK_BUTTON5_MASK 
	);
	
	// Don't start the save if the user is holding a mouse button
	if ((mask & anyButton) != 0) {
		return;
	}

	// Check, if changes have been made since the last autosave
	if (_changes == Node_getMapFile(GlobalSceneGraph().root())->changes()) {
		return;
	}

	_changes = Node_getMapFile(GlobalSceneGraph().root())->changes();

	// Stop the timer before saving
	stopTimer();

	if (_enabled) {
		// only snapshot if not working on an unnamed map
		if (_snapshotsEnabled && !GlobalMap().isUnnamed()) {
			try {
				saveSnapshot();
			}
			catch (boost::filesystem::filesystem_error f) {
				globalErrorStream() << "AutoSaver::saveSnapshot: " << f.what() << "\n";
			}
		}
		else {
			if (GlobalMap().isUnnamed()) {
				// Get the maps path (within the mod path)
				std::string autoSaveFilename = GlobalRegistry().get(RKEY_MAP_PATH);
				
				// Try to create the map folder, in case there doesn't exist one 
				os::makeDirectory(autoSaveFilename);
				
				// Append the "autosave.map" to the filename
				autoSaveFilename += "autosave.";
				autoSaveFilename += GlobalRegistry().get(RKEY_MAP_EXTENSION);
				
				globalOutputStream() << "Autosaving unnamed map to " << autoSaveFilename.c_str() << "\n";
				
				// Invoke the save call
				GlobalMap().saveDirect(autoSaveFilename);
			}
			else {
				// Construct the new extension (e.g. ".autosave.map")
				std::string newExtension = ".autosave.";
				newExtension += GlobalRegistry().get(RKEY_MAP_EXTENSION);
				
				try {
					// create the new autosave filename by changing the extension
					Path autoSaveFilename = boost::filesystem::change_extension(
							Path(GlobalMap().getMapName(), boost::filesystem::native), 
							newExtension
						);
					
					globalOutputStream() << "Autosaving map to " << autoSaveFilename.string().c_str() << "\n";
				
					// Invoke the save call
					GlobalMap().saveDirect(autoSaveFilename.string());
				}
				catch (boost::filesystem::filesystem_error f) {
					globalErrorStream() << "AutoSaver: " << f.what() << "\n";
				}
			}
		}
	}
	else {
		globalOutputStream() << "Autosave skipped...\n";
	}
	
	// Re-start the timer after saving has finished
	startTimer();
}

void AutoMapSaver::constructPreferences() {
	// Add a page to the given group
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Settings/Autosave");
	
	// Add the checkboxes and connect them with the registry key and the according observer 
	page->appendCheckBox("", "Enable Autosave", RKEY_AUTOSAVE_ENABLED);
	page->appendSlider("Autosave Interval (in minutes)", RKEY_AUTOSAVE_INTERVAL, TRUE, 5, 1, 61, 1, 1, 1);
	
	page->appendCheckBox("", "Save Snapshots", RKEY_AUTOSAVE_SNAPSHOTS_ENABLED);
	page->appendEntry("Snapshot folder (relative to map folder)", RKEY_AUTOSAVE_SNAPSHOTS_FOLDER);
	page->appendEntry("Max Snapshot Folder size (MB)", RKEY_AUTOSAVE_MAX_SNAPSHOT_FOLDER_SIZE);
}

gboolean AutoMapSaver::onIntervalReached(gpointer data) {
	// Cast the passed pointer onto a self-pointer
	AutoMapSaver* self = reinterpret_cast<AutoMapSaver*>(data);
	
	self->checkSave();
	
	// Return true, so that the timer gets called again
	return true;
}

AutoMapSaver& AutoSaver() {
	static AutoMapSaver _autoSaver;
	return _autoSaver;
}

} // namespace map
