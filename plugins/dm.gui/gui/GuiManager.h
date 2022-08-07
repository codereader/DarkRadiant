#pragma once

#include "igui.h"
#include "util/Noncopyable.h"
#include <map>
#include "ifilesystem.h"
#include "string/string.h"
#include "parser/ThreadedDefLoader.h"

namespace gui
{

namespace
{
	const std::string GUI_DIR("guis/readables/");
	const std::string GUI_EXT("gui");
}

class Gui;
typedef std::shared_ptr<Gui> GuiPtr;

/**
 * greebo: This manager keeps track of all the loaded GUIs,
 * including parsing the .gui files on demand.
 */
class GuiManager :
	public IGuiManager,
	public util::Noncopyable	
{
private:
	struct GuiInfo
	{
		// The type of this Gui (NOT_LOADED_YET by default)
		GuiType type;

		// the cached GUI pointer, can be NULL if load failed
		GuiPtr gui;

		GuiInfo() :
			type(NOT_LOADED_YET)
		{}

		GuiInfo(const GuiPtr& gui_, GuiType type_) :
			type(type_),
			gui(gui_)
		{}
	};

	// The table of all loaded Gui, sorted by VFS path
	typedef std::map<std::string, GuiInfo> GuiInfoMap;
	GuiInfoMap _guis;

    parser::ThreadedDefLoader<void> _guiLoader;

	// A List of all the errors occuring lastly.
	StringList _errorList;

public:
	GuiManager();

	// Gets a GUI from the given VFS path, parsing it on demand
	// Returns NULL if the GUI couldn't be found or loaded.
	IGuiPtr getGui(const std::string& guiPath) override;

	// Returns the number of known GUIs (or GUI paths)
	std::size_t getNumGuis() override;

	// Traverse all known GUIs using the given Visitor
	void foreachGui(Visitor& visitor) override;

	// Returns the GUI appearance type for the given GUI path
	GuiType getGuiType(const std::string& guiPath) override;

	// Reload the gui
	void reloadGui(const std::string& guiPath) override;

	// Returns the _errorList for use in a GUI.
	const StringList& getErrorList() override { return _errorList; }

    // Clears out the GUIs and reloads them
    void reloadGuis() override;

	// RegisterableModule
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	void init();
	void clear();

    // Searches the VFS for all available GUI definitions
    void findGuis();

    void ensureGuisLoaded();

	GuiType determineGuiType(const GuiPtr& gui);

	GuiPtr loadGui(const std::string& guiPath);

    // Used by findGuis()
    void registerGui(const std::string& guiPath);
};

} // namespace
