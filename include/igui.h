#pragma once

#include <vector>
#include <string>
#include "imodule.h"

namespace gui
{

class Gui;
typedef std::shared_ptr<Gui> GuiPtr;

enum GuiType
{
	NOT_LOADED_YET,		// no attempt to load the GUI has been made
	UNDETERMINED,		// not checked yet for type
	ONE_SIDED_READABLE,	// 1-sided
	TWO_SIDED_READABLE,	// 2-sided
	NO_READABLE,		// not a readable
	IMPORT_FAILURE,		// failed to load
	FILE_NOT_FOUND,		// file doesn't exist
};

/**
* greebo: Interface managing the idTech4 GUI files,
* keeping track of all the loaded GUIs,
* parsing the .gui files on demand.
*/
class IGuiManager :
	public RegisterableModule
{
public:
	typedef std::vector<std::string> StringList;

	// A visitor class used to traverse all known GUIs by path
	class Visitor
	{
	public:
		virtual ~Visitor() {}

		virtual void visit(const std::string& guiPath, const GuiType& guiType) = 0;
	};

public:
	virtual ~IGuiManager() {}

	// Gets a GUI from the given VFS path, parsing it on demand
	// Returns NULL if the GUI couldn't be found or loaded.
	virtual GuiPtr getGui(const std::string& guiPath) = 0;

	// Returns the number of known GUIs (or GUI paths)
	virtual std::size_t getNumGuis() = 0;

	// Traverse all known GUIs using the given Visitor
	virtual void foreachGui(Visitor& visitor) = 0;

	// Returns the GUI appearance type for the given GUI path
	virtual GuiType getGuiType(const std::string& guiPath) = 0;

	// Reload the gui
	virtual void reloadGui(const std::string& guiPath) = 0;

	// Returns the _errorList for use in a GUI.
	virtual const StringList& getErrorList() = 0;

	// Clears out the GUIs and reloads them
	virtual void reloadGuis() = 0;
};

}

const char* const MODULE_GUIMANAGER("GuiManager");

// Application-wide Accessor to the global GUI manager
inline gui::IGuiManager& GlobalGuiManager()
{
	// Cache the reference locally
	static gui::IGuiManager& _manager(
		*std::static_pointer_cast<gui::IGuiManager>(
			module::GlobalModuleRegistry().getModule(MODULE_GUIMANAGER))
	);
	return _manager;
}
