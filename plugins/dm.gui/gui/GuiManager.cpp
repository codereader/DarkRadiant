#include "GuiManager.h"

#include "iarchive.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "parser/CodeTokeniser.h"
#include "generic/callback.h"

#include "Gui.h"

namespace gui
{

const GuiManager::GuiMap&  GuiManager::refreshGuiDefinitions()
{
	_guis.clear();
	_errorList.clear();
	GlobalFileSystem().forEachFile(
		GUI_DIR,
		GUI_EXT,
		makeCallback1(*this),
		99);

	return _guis;
}

const GuiManager::GuiMap& GuiManager::getGuiDefinitions()
{
	static GuiMap guis = refreshGuiDefinitions();	// Make sure this is only called once.
	if (_guis.empty())
		throw std::runtime_error("GuiMap is empty.");
	return _guis;
}

const GuiManager::GuiAppearance GuiManager::checkGuiAppearance(const std::string& guiPath)
{
	GuiPtr request = getGui(guiPath);
	if (!request)
		return IMPORT_FAILURE;
	return checkGuiAppearance(request);
}

const GuiManager::GuiAppearance GuiManager::checkGuiAppearance(const GuiPtr& gui)
{
	if (gui->findWindowDef("title"))
		return ONE_SIDED_READABLE;
	if (gui->findWindowDef("leftTitle"))
		return TWO_SIDED_READABLE;
	return NO_READABLE;
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
