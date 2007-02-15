#ifndef MAPFILEMANAGER_H_
#define MAPFILEMANAGER_H_

#include <string>

namespace map
{

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
	// Last directory used to load/save a file. This will be the default in 
	// future dialogs
	std::string _lastDir;

private:

	// Constructor
	MapFileManager();

	// Static instance owner
	static MapFileManager& getInstance();

	// Utility function to display a file chooser and return the selected path
	std::string selectFile(bool open);

public:

	/* STATIC INTERFACE */

	/**
	 * Query the user for a map file to load.
	 * 
	 * @returns
	 * The full path of the file to load, or the empty string if no selection
	 * was made.
	 */
	 static std::string getLoadFilename();
	 
	 /**
	  * Query the user for a map file to save.
	  * 
	  * @returns
	  * The full path of the file to save, or the empty string if no selection
	  * was made.
	  */
	static std::string getSaveFilename();

};

}

#endif /*MAPFILEMANAGER_H_*/
