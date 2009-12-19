#include "MapFileManager.h"

#include "iregistry.h"
#include "ifiletypes.h"
#include "imainframe.h"
#include "modulesystem/ApplicationContextImpl.h"
#include "gtkutil/FileChooser.h"
#include "gtkutil/IConv.h"
#include "os/path.h"
#include "MapFileChooserPreview.h"
#include <gtk/gtkwidget.h>

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
	gtkutil::FileChooser fileChooser(GTK_WIDGET(GlobalMainFrame().getTopLevelWindow()),
		title, open, false, type, defaultExt);

	fileChooser.setCurrentFile(defaultFile);
	fileChooser.setCurrentPath(_lastDirs[type]);

	// For prefabs, add a preview widget
	if (open && type == "prefab") {
		// Instantiate a new preview object
		MapFileChooserPreviewPtr preview(new MapFileChooserPreview());

		// attach the preview object
		fileChooser.attachPreview(preview);
	}

	std::string filePath = fileChooser.display();

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
