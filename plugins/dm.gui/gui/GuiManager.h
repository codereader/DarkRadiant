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

	enum GuiAppearance
	{
		ONE_SIDED_READABLE,
		TWO_SIDED_READABLE,
		NO_READABLE,
		IMPORT_FAILURE,
	};

private:
	// The table of all loaded Gui, sorted by VFS path
	GuiMap _guis;

	// A List of all the errors occuring lastly.
	StringList _errorList;
public:
	// Gets a GUI from the given VFS path, parsing it on demand
	// Returns NULL if the GUI couldn't be found or loaded.
	GuiPtr getGui(const std::string& guiPath);

	// Operator used for callback by refreshGuiDefinitions.
	void operator() (const std::string& guiPath) { loadGui( GUI_DIR + guiPath ); }

	// Getter for _guis. Throws runtime_error if _guis is empty.
	const GuiMap& getGuiDefinitions();

	// Checks the appearance of a given guiPath or gui. If an IMPORT_FAIlURE is the case,
	// a proper error-message is to be found in the last element of getErrorList().
	const GuiAppearance checkGuiAppearance(const std::string& guiPath);
	const GuiAppearance checkGuiAppearance(const GuiPtr& gui);

	// Returns the _errorList for use in a GUI.
	const StringList& getErrorList() { return _errorList; }

	// Provides access to the singleton
	static GuiManager& Instance();

	// Required typedef for the Callback to work.
	typedef const std::string& first_argument_type;

private:
	GuiPtr loadGui(const std::string& guiPath);

	// Retrieves all available GUI definitions and stores them in _guis.
	const GuiMap& refreshGuiDefinitions();
};

} // namespace

#endif // GuiManager_h__
