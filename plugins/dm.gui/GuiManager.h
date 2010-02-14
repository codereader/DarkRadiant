#ifndef GuiManager_h__
#define GuiManager_h__

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <map>

namespace gui
{

class Gui;
typedef boost::shared_ptr<Gui> GuiPtr;

/**
 * greebo: This manager keeps track of all the loaded GUIs, 
 * including parsing the .gui files on demand.
 */
class GuiManager :
	public boost::noncopyable
{
private:
	// The table of all loaded Gui, sorted by VFS path
	typedef std::map<std::string, GuiPtr> GuiMap;
	GuiMap _guis;

public:
	// Gets a GUI from the given VFS path, parsing it on demand
	// Returns NULL if the GUI couldn't be found or loaded.
	GuiPtr getGui(const std::string& guiPath);

	// Provides access to the singleton
	static GuiManager& Instance();

private:
	GuiPtr loadGui(const std::string& guiPath);
	std::string loadGuiSource(const std::string& guiPath);
	std::string loadFile(const std::string& vfsPath);
};

} // namespace

#endif // GuiManager_h__
