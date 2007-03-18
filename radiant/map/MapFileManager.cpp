#include "MapFileManager.h"

#include "iradiant.h"
#include "ifiletypes.h"
#include "mainframe.h"
#include "gtkutil/filechooser.h"
#include "os/path.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>

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
std::string MapFileManager::selectFile(bool open, 
	const std::string& title, const std::string& type) 
{
	// Display a file chooser dialog to get a new path
	std::string filePath = 
		os::standardPath(file_dialog(GTK_WIDGET(MainFrame_getWindow()), 
								     open, 
									 title, 
									 _lastDir, 
									 type.c_str()));

	// Get the first extension from the list of possible patterns (e.g. *.pfb or *.map)
	ModuleTypeListPtr typeList = GlobalFiletypes().getTypesFor(type);
	std::string defaultExt = typeList->begin()->filePattern.pattern;
	// remove the * from the pattern "*.pfb" >>> ".pfb"
	boost::algorithm::erase_all(defaultExt, "*");

	// If a filename was chosen, update the last path
	if (!filePath.empty())
		_lastDir = filePath.substr(0, filePath.rfind("/"));
		
	// Return the chosen file. If this is a save operation and the chosen path
	// does not end in the default extension (".map"), add it here.
	if (!open												// save operation 
		&& !filePath.empty() 								// valid filename
		&& !boost::algorithm::iends_with(filePath, defaultExt)) // no map extension
	{
		return filePath + defaultExt;
	}
	else
	{
		return filePath;
	}
}

/* PUBLIC INTERFACE METHODS */

// Static method to get a load filename
std::string MapFileManager::getMapFilename(bool open, 
										   const std::string& title, 
										   const std::string& type) 
{
	return getInstance().selectFile(open, title, type);
}

} // namespace map
