#include "MapFileManager.h"

#include "iregistry.h"
#include "ifiletypes.h"
#include "modulesystem/ApplicationContextImpl.h"
#include "mainframe.h"
#include "gtkutil/filechooser.h"
#include "gtkutil/IConv.h"
#include "os/path.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>

namespace map
{
	
MapFileManager::MapFileManager() {
	// Load the default values
	_lastDirs["map"] = GlobalRegistry().get(RKEY_MAP_PATH);
	_lastDirs["prefab"] = GlobalRegistry().get(RKEY_PREFAB_PATH);
}

// Instance owner method
MapFileManager& MapFileManager::getInstance() {
	static MapFileManager _instance;
	return _instance;	
}

// Utility method to select a map file
std::string MapFileManager::selectFile(bool open, 
	const std::string& title, const std::string& type, const std::string& defaultFile) 
{
	// Check, if the lastdir contains at least anything and load
	// the default map path if it's empty
	if (_lastDirs.find(type) == _lastDirs.end()) {
		// Default to the map path, if the type is not yet associated
		_lastDirs[type] = GlobalRegistry().get(RKEY_MAP_PATH);
	}
	
	// Get the first extension from the list of possible patterns (e.g. *.pfb or *.map)
	ModuleTypeListPtr typeList = GlobalFiletypes().getTypesFor(type);
	std::string defaultExt = typeList->begin()->filePattern.pattern;
	// remove the * from the pattern "*.pfb" >>> ".pfb"
	boost::algorithm::erase_all(defaultExt, "*");
	
	// Display a file chooser dialog to get a new path
	std::string filePath = 
		file_dialog(GTK_WIDGET(MainFrame_getWindow()), 
				     open, 
					 title, 
					 _lastDirs[type], 
					 type,
					 defaultExt,
                     defaultFile);

	// If a filename was chosen, update the last path
	if (!filePath.empty()) {
		_lastDirs[type] = filePath.substr(0, filePath.rfind("/"));
	}

	return gtkutil::IConv::localeFromUTF8(filePath);
}

/* PUBLIC INTERFACE METHODS */

// Static method to get a load filename
std::string MapFileManager::getMapFilename(bool open, 
										   const std::string& title, 
										   const std::string& type,
                                           const std::string& defaultFile) 
{
	return getInstance().selectFile(open, title, type, defaultFile);
}

} // namespace map
