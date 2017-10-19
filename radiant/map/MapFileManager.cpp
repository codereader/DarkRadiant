#include "MapFileManager.h"

#include "i18n.h"
#include "iregistry.h"
#include "ifiletypes.h"
#include "imainframe.h"
#include "modulesystem/ApplicationContextImpl.h"
#include "wxutil/FileChooser.h"
#include "wxutil/IConv.h"
#include "os/path.h"

namespace map
{

MapFileManager::MapFileManager()
{
	// Load the default values
	_lastDirs[filetype::TYPE_MAP] = GlobalRegistry().get(RKEY_MAP_PATH);
	_lastDirs[filetype::TYPE_PREFAB] = GlobalRegistry().get(RKEY_PREFAB_PATH);
}

// Instance owner method
MapFileManager& MapFileManager::getInstance()
{
	static MapFileManager _instance;
	return _instance;
}

void MapFileManager::registerFileTypes()
{
	// Register the map file extension in the FileTypeRegistry
	GlobalFiletypes().registerPattern(filetype::TYPE_MAP, FileTypePattern(_("Map"), "map", "*.map"));
	GlobalFiletypes().registerPattern(filetype::TYPE_REGION, FileTypePattern(_("Region"), "reg", "*.reg"));
	GlobalFiletypes().registerPattern(filetype::TYPE_PREFAB, FileTypePattern(_("Prefab"), "pfb", "*.pfb"));
}

// Utility method to select a map file
MapFileSelection MapFileManager::selectFile(bool open,
	const std::string& title, const std::string& type, const std::string& defaultFile)
{
	MapFileSelection fileInfo;

	// Check, if the lastdir contains at least anything and load
	// the default map path if it's empty
	if (_lastDirs.find(type) == _lastDirs.end())
	{
		// Default to the map path, if the type is not yet associated
		_lastDirs[type] = GlobalRegistry().get(RKEY_MAP_PATH);
	}

	// Get the first extension from the list of possible patterns (e.g. *.pfb or *.map)
	FileTypePatterns patterns = GlobalFiletypes().getPatternsForType(type);

	std::string defaultExt = "";

	if (!patterns.empty())
	{
		defaultExt = "." + patterns.begin()->extension; // ".map"
	}

	// Display a file chooser dialog to get a new path
	wxutil::FileChooser fileChooser(title, open, type, defaultExt);

	fileChooser.setCurrentFile(defaultFile);
	fileChooser.setCurrentPath(_lastDirs[type]);

	std::string filePath = fileChooser.display();

	// If a filename was chosen, update the last path
	if (!filePath.empty())
	{
		_lastDirs[type] = filePath.substr(0, filePath.rfind("/"));
	}

	fileInfo.fullPath = filePath;
	fileInfo.mapFormatName = fileChooser.getSelectedMapFormat();
	fileInfo.mapFormat = GlobalMapFormatManager().getMapFormatByName(fileInfo.mapFormatName);

	return fileInfo;
}

/* PUBLIC INTERFACE METHODS */

MapFileSelection MapFileManager::getMapFileSelection(bool open,
										 const std::string& title,
										 const std::string& type,
										 const std::string& defaultFile)
{
	return getInstance().selectFile(open, title, type, defaultFile);
}

} // namespace map
