#ifndef GuiManager_h__
#define GuiManager_h__

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include "ifilesystem.h"
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
	NOT_LOADED_YET,		// no attempt to load the GUI has been made
	UNDETERMINED,		// not checked yet for type
	ONE_SIDED_READABLE,	// 1-sided
	TWO_SIDED_READABLE,	// 2-sided
	NO_READABLE,		// not a readable
	IMPORT_FAILURE,		// failed to load
	FILE_NOT_FOUND,		// file doesn't exist
};

class Gui;
typedef boost::shared_ptr<Gui> GuiPtr;

/**
 * greebo: This manager keeps track of all the loaded GUIs,
 * including parsing the .gui files on demand.
 */
class GuiManager :
	public VirtualFileSystem::Visitor,
	public boost::noncopyable
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

	// A List of all the errors occuring lastly.
	StringList _errorList;

public:
	// Gets a GUI from the given VFS path, parsing it on demand
	// Returns NULL if the GUI couldn't be found or loaded.
	GuiPtr getGui(const std::string& guiPath);

	// Operator used for callback by refreshGuiDefinitions.
	void visit(const std::string& guiPath);

	// Returns the number of known GUIs (or GUI paths)
	std::size_t getNumGuis() const;

	// Traverse all known GUIs using the given Visitor
	void foreachGui(Visitor& visitor);

	// Returns the GUI appearance type for the given GUI path
	GuiType getGuiType(const std::string& guiPath);

	// Reload the gui
	void reloadGui(const std::string& guiPath);

	// Returns the _errorList for use in a GUI.
	const StringList& getErrorList() { return _errorList; }

	// Provides access to the singleton
	static GuiManager& Instance();

	// Searches the VFS for all available GUI definitions
	void findGuis();

	// Clears all internal objects
	void clear();

private:
	GuiType determineGuiType(const GuiPtr& gui);

	GuiPtr loadGui(const std::string& guiPath);
};

} // namespace

#endif // GuiManager_h__
