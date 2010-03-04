#include "GuiManager.h"

#include "iarchive.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "parser/CodeTokeniser.h"
#include "generic/callback.h"

#include "Gui.h"

namespace gui
{

GuiManager::GuiManager() :
	_guiTypesLoaded(false)
{}

void GuiManager::operator() (const std::string& guiPath)
{ 
	GuiPtr gui = loadGui(GUI_DIR + guiPath);

	if (gui == NULL)
	{
		_guiAppearance[GUI_DIR + guiPath] = IMPORT_FAILURE;
		return;
	}

	// TODO: Find a better way of distinguishing GUIs
	if (gui->findWindowDef("title") != NULL)
	{
		_guiAppearance[GUI_DIR + guiPath] = ONE_SIDED_READABLE;
	}
	else if (gui->findWindowDef("leftTitle") != NULL)
	{
		_guiAppearance[GUI_DIR + guiPath] = TWO_SIDED_READABLE;
	}
	else
	{
		_guiAppearance[GUI_DIR + guiPath] = NO_READABLE;
	}
}

GuiAppearance GuiManager::getGuiAppearance(const std::string& guiPath)
{
	buildGuiTypeMap();

	GuiAppearanceMap::const_iterator i = _guiAppearance.find(guiPath);

	return (i != _guiAppearance.end()) ? i->second : NO_READABLE;
}

void GuiManager::buildGuiTypeMap()
{
	if (_guiTypesLoaded) return;

	_guiTypesLoaded = true;

	_guiAppearance.clear();

	GlobalFileSystem().forEachFile(
		GUI_DIR,
		GUI_EXT,
		makeCallback1(*this),
		99);
}

GuiPtr GuiManager::getGui(const std::string& guiPath)
{
	GuiMap::iterator i = _guis.find(guiPath);

	if (i != _guis.end())
	{
		return i->second;
	}

	// GUI not buffered, try to load afresh
	return loadGui(guiPath);
}

const GuiManager::GuiAppearanceMap& GuiManager::getGuiAppearanceMap()
{
	buildGuiTypeMap();

	return _guiAppearance;
}

GuiPtr GuiManager::loadGui(const std::string& guiPath)
{
	ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(guiPath);

	if (file == NULL)
	{
		std::string errMSG = "Could not open file: " + guiPath + "\n";
		_errorList.push_back(errMSG);
		globalErrorStream() << errMSG;

		return GuiPtr();
	}

	// Construct a Code Tokeniser, which is able to handle #includes
	try
	{
		std::string whiteSpace = std::string(parser::WHITESPACE) + ",";
		parser::CodeTokeniser tokeniser(file, whiteSpace.c_str(), "{}(),;");

		GuiPtr gui = Gui::createFromTokens(tokeniser);
		if (gui)
			_guis[guiPath] = gui;

		return gui;
	}
	catch (parser::ParseException& p)
	{
		std::string errMSG = "Error while parsing " + guiPath + ": " + p.what() + "\n";
		_errorList.push_back(errMSG);
		globalErrorStream() << errMSG;

		return GuiPtr();
	}
}

GuiManager& GuiManager::Instance()
{
	static GuiManager _instance;
	return _instance;
}

} // namespace gui
