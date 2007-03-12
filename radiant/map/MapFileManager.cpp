#include "MapFileManager.h"

#include "qerplugin.h"
#include "mainframe.h"
#include "gtkutil/filechooser.h"
#include "os/path.h"

#include <boost/algorithm/string/predicate.hpp>

namespace map
{

// Private constructor
MapFileManager::MapFileManager()
: _lastDir(GlobalRadiant().getMapsPath())
{ }

// Instance owner method
MapFileManager& MapFileManager::getInstance() {
	static MapFileManager _instance;
	return _instance;	
}

// Utility method to select a map file
std::string MapFileManager::selectFile(bool open, const std::string& title) {

	// Display a file chooser dialog to get a new path
	std::string filePath = 
		os::standardPath(file_dialog(GTK_WIDGET(MainFrame_getWindow()), 
								     open, 
									 title, 
									 _lastDir, 
									 "map"));

	// If a filename was chosen, update the last path
	if (!filePath.empty())
		_lastDir = filePath.substr(0, filePath.rfind("/"));
		
	// Return the chosen file. If this is a save operation and the chosen path
	// does not end in ".map", add it here.
	if (!open												// save operation 
		&& !filePath.empty() 								// valid filename
		&& !boost::algorithm::iends_with(filePath, ".map")) // no map extension
	{
		return filePath + ".map";
	}
	else
	{
		return filePath;
	}
}

/* PUBLIC INTERFACE METHODS */

// Static method to get a load filename
std::string MapFileManager::getMapFilename(bool open, 
										   const std::string& title) 
{
	return getInstance().selectFile(open, title);
}

} // namespace map
