#ifndef GuiManager_h__
#define GuiManager_h__

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include "string/string.h"
#include <vector>

namespace gui
{

namespace
{
	const std::string GUI_DIR("guis/readables/");
	const std::string GUI_EXT("gui");
}

enum GuiType
{
	ONE_SIDED_READABLE,
	TWO_SIDED_READABLE,
	NO_READABLE,
	IMPORT_FAILURE,
	FILE_NOT_FOUND,
};

class Gui;
typedef boost::shared_ptr<Gui> GuiPtr;

/**
 * greebo: This manager keeps track of all the loaded GUIs, 
 * including parsing the .gui files on demand.
 */
class GuiManager :
	public boost::noncopyable
{
public:
	typedef std::map<std::string, GuiPtr> GuiMap;
	typedef std::vector<std::string> StringList;

	typedef std::map<std::string, GuiType> GuiTypeMap;

private:
	// The table of all loaded Gui, sorted by VFS path
	GuiMap _guis;

	// A List of all the errors occuring lastly.
	StringList _errorList;
	
	GuiTypeMap _guiType;

	bool _guiTypesLoaded;

	GuiManager();

public:
	// Gets a GUI from the given VFS path, parsing it on demand
	// Returns NULL if the GUI couldn't be found or loaded.
	GuiPtr getGui(const std::string& guiPath);

	// Operator used for callback by refreshGuiDefinitions.
	void operator() (const std::string& guiPath);

	// Getter for GUI types
	const GuiTypeMap& getGuiTypeMap();

	// Returns the GUI appearance type for the given GUI path
	GuiType getGuiType(const std::string& guiPath);

	// Returns the _errorList for use in a GUI.
	const StringList& getErrorList() { return _errorList; }

	// Provides access to the singleton
	static GuiManager& Instance();

	// Required typedef for the Callback to work.
	typedef const std::string& first_argument_type;

private:
	// Searches the VFS for all available GUI definitions
	void buildGuiTypeMap();

	GuiPtr loadGui(const std::string& guiPath);
};

} // namespace

#endif // GuiManager_h__
