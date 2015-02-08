#include "GuiManager.h"

#include "iarchive.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "parser/CodeTokeniser.h"

#include "Gui.h"

namespace gui
{

GuiManager::GuiManager() :
    _guisLoaded(false)
{}

void GuiManager::registerGui(const std::string& guiPath)
{
	// Just store the path in the map, for later reference
	_guis.insert(GuiInfoMap::value_type(GUI_DIR + guiPath, GuiInfo()));
}

std::size_t GuiManager::getNumGuis()
{
    ensureGuisLoaded();

	return _guis.size();
}

void GuiManager::foreachGui(Visitor& visitor)
{
    ensureGuisLoaded();

	for (GuiInfoMap::iterator i = _guis.begin(); i != _guis.end(); ++i)
	{
		visitor.visit(i->first, i->second.type);
	}
}

void GuiManager::reloadGui(const std::string& guiPath)
{
	GuiPtr gui = loadGui(guiPath);
	determineGuiType(gui);
}

GuiType GuiManager::getGuiType(const std::string& guiPath)
{
	// Get the GUI (will load the file if necessary)
	GuiPtr gui = getGui(guiPath);

	GuiInfoMap::iterator found = _guis.find(guiPath);

	if (found == _guis.end())
	{
		return FILE_NOT_FOUND;
	}

	// Gui Info found, determine readable type if necessary
	if (found->second.type == UNDETERMINED)
	{
		found->second.type = determineGuiType(found->second.gui);
	}

	return found->second.type;
}

GuiType GuiManager::determineGuiType(const GuiPtr& gui)
{
	if (gui)
	{
		// TODO: Find a better way of distinguishing GUIs
		if (gui->findWindowDef("body") != NULL)
		{
			return ONE_SIDED_READABLE;
		}
		else if (gui->findWindowDef("leftBody") != NULL)
		{
			return TWO_SIDED_READABLE;
		}
	}
	else
	{
		return IMPORT_FAILURE;
	}

	return NO_READABLE;
}

void GuiManager::init()
{
    _loadResult = std::async(std::launch::async, [this]()->bool
    {
        findGuis();
        return true;
    });
}

void GuiManager::reloadGuis()
{
    clear();
    init();
}

void GuiManager::findGuis()
{
    _errorList.clear();
    _guis.clear();

    // Traverse the file system, using this class as callback
    GlobalFileSystem().forEachFile(GUI_DIR, GUI_EXT, [&](const std::string& filename)
    {
        registerGui(filename);
    }, 99);

    rMessage() << "[GuiManager]: Found " << _guis.size() << " guis." << std::endl;
}

void GuiManager::clear()
{
    // Wait for the thread to finish
    if (_loadResult.valid())
    {
        _loadResult.get();
    }

	_guis.clear();
	_errorList.clear();
}

GuiPtr GuiManager::getGui(const std::string& guiPath)
{
    ensureGuisLoaded();

	GuiInfoMap::iterator i = _guis.find(guiPath);

	// Path existent?
	if (i != _guis.end())
	{
		// Found in the map, load if not yet attempted
		if (i->second.type == NOT_LOADED_YET)
		{
			loadGui(guiPath);
		}

		return i->second.gui;
	}

	// GUI not buffered, try to load afresh
	return loadGui(guiPath);
}

void GuiManager::ensureGuisLoaded()
{
    if (!_guisLoaded && !_loadResult.valid())
    {
        // No GUIs loaded and no one currently looking for them

        // Launch a new thread
        _loadResult = std::async(std::launch::async, [this]()->bool
        {
            findGuis();
            return true;
        });
    }

    // If the thread is still running, block until it's done
    if (_loadResult.valid())
    {
        _guisLoaded = _loadResult.get();
        _loadResult = std::future<bool>();
    }

    // When reaching this point, the GUIs should be loaded
    assert(_guisLoaded);
}

GuiPtr GuiManager::loadGui(const std::string& guiPath)
{
    ensureGuisLoaded();

	// Insert a new entry in the map, if necessary
	std::pair<GuiInfoMap::iterator, bool> result = _guis.insert(
		GuiInfoMap::value_type(guiPath, GuiInfo())
	);

	GuiInfo& info = result.first->second;

	ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(guiPath);

	if (file == NULL)
	{
		std::string errMSG = "Could not open file: " + guiPath + "\n";

		_errorList.push_back(errMSG);
		rError() << errMSG;

		info.type = FILE_NOT_FOUND;

		return GuiPtr();
	}

	// Construct a Code Tokeniser, which is able to handle #includes
	try
	{
		std::string whiteSpace = std::string(parser::WHITESPACE) + ",";
		parser::CodeTokeniser tokeniser(file, whiteSpace.c_str(), "{}(),;");

		info.gui = Gui::createFromTokens(tokeniser);
		info.type = UNDETERMINED;

		return info.gui;
	}
	catch (parser::ParseException& p)
	{
		std::string errMSG = "Error while parsing " + guiPath + ": " + p.what() + "\n";
		_errorList.push_back(errMSG);
		rError() << errMSG;

		info.type = IMPORT_FAILURE;
		return GuiPtr();
	}
}

GuiManager& GuiManager::Instance()
{
	static GuiManager _instance;
	return _instance;
}

} // namespace gui
