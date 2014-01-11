#pragma once

#include <string>
#include <map>
#include "imapformat.h"

namespace map
{

class MapFileSelection
{
public:
	// The full path of the selected file
	std::string fullPath;

	// For save dialogs, a specific map format might have been selected
	std::string mapFormatName;

	// The reference to the named mapformat above. Can be an empty pointer.
	MapFormatPtr mapFormat;
};

/**
 * Service class to handle the directory and filenames used for loading and
 * saving maps. Provides persistence for the maps directory, so that the
 * default save directory is the same as the last load directory, and handles
 * querying the user for a load/save filename using GTK dialogs.
 *
 * The class exists as a singleton, with all public interface exposed via
 * static methods.
 */
class MapFileManager
{
	// The mapping between type and last viewed path
	// like "map" => "/usr/local/game/doom3/maps/"
	// and "prefab" => "~/.doom3/darkmod/prefabs/"
	typedef std::map<std::string, std::string> PathMap;
	PathMap _lastDirs;

private:

	// Static instance owner
	static MapFileManager& getInstance();

	// Utility function to display a file chooser and return the selected path
	MapFileSelection selectFile(bool open, const std::string& title, const std::string& type, const std::string& defaultFile);

public:
	// Constructor, loads the default map and prefab paths
	MapFileManager();

	/**
	 * Query the user for a map file to load or save.
	 *
	 * @param open
	 * Whether this is an open operation or a save operation (changes file
	 * dialog behaviour).
	 *
	 * @param title
	 * The title to display on the dialog, such as "Open map" or "Export
	 * selection".
	 *
	 * @param type: the file type to be loaded ("map", "prefab" or "region")
	 *
	 * @returns
	 * The info structure of the file selection, the member fullPath of which will
	 * be empty string if no selection was made.
	 */
   static MapFileSelection getMapFileSelection(bool open,
										 const std::string& title,
										 const std::string& type,
										 const std::string& defaultFile = "");

   // Register the file types during startup
   static void registerFileTypes();
};

}
