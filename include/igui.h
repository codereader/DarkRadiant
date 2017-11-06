#pragma once

#include <vector>
#include <string>
#include "imodule.h"

namespace gui
{

class GuiWindowDef;
typedef std::shared_ptr<GuiWindowDef> GuiWindowDefPtr;

/**
* greebo: This class represents a single D3 GUI. It holds all
* the windowDefs and the source code behind.
*/
class IGui
{
public:
	virtual ~IGui() {}

	virtual const GuiWindowDefPtr& getDesktop() const = 0;
	virtual void setDesktop(const GuiWindowDefPtr& newDesktop) = 0;

	// Sets the given state variable (gui::<key> = <value>)
	virtual void setStateString(const std::string& key, const std::string& value) = 0;

	// Returns the state string "gui::<key>" or an empty string if non-existent
	virtual std::string getStateString(const std::string& key) = 0;

	// Sets up the time of the entire GUI (all windowDefs)
	virtual void initTime(const std::size_t time) = 0;

	// "Think" routine, advances all active windowDefs (where notime == false)
	virtual void update(const std::size_t timestep) = 0;

	// Returns a reference to the named windowDef, returns NULL if not found
	virtual GuiWindowDefPtr findWindowDef(const std::string& name) = 0;

	// Called by the GuiRenderer to re-compile text VBOs, etc.
	virtual void pepareRendering() = 0;
};
typedef std::shared_ptr<IGui> IGuiPtr;

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
	virtual IGuiPtr getGui(const std::string& guiPath) = 0;

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
