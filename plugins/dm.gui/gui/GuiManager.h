#ifndef GuiManager_h__
#define GuiManager_h__

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include "string/string.h"

namespace gui
{

namespace
{
	const std::string GUI_DIR("guis/");
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

private:
	// The table of all loaded Gui, sorted by VFS path
	GuiMap _guis;

public:
	// Gets a GUI from the given VFS path, parsing it on demand
	// Returns NULL if the GUI couldn't be found or loaded.
	GuiPtr getGui(const std::string& guiPath);
	void operator() (const std::string& guiPath) { getGui( GUI_DIR + guiPath ); }

	// Retrieves all available GUI definitions and stores them in _guis.
	void refreshGuiDefinitions();

	// Getter for _guis. Throws runtime_error if _guis is empty.
	const GuiMap& getGuiDefinitions();

	// Provides access to the singleton
	static GuiManager& Instance();

	// Required typedef for the Callback to work.
	typedef const std::string& first_argument_type;

private:
	GuiPtr loadGui(const std::string& guiPath);
};

} // namespace

#endif // GuiManager_h__
