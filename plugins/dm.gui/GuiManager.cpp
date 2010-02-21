#include "GuiManager.h"

#include "iarchive.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "parser/CodeTokeniser.h"

#include "Gui.h"

namespace gui
{

GuiPtr GuiManager::getGui(const std::string& guiPath)
{
	GuiMap::iterator i = _guis.find(guiPath);

	if (i != _guis.end())
	{
		return i->second;
	}

	// GUI not buffered, try to load afresh
	GuiPtr gui = loadGui(guiPath);

	if (gui != NULL)
	{
		// TODO: Insert gui into cache
	}
	else
	{
		globalWarningStream() << "Could not find GUI: " << guiPath << std::endl;
	}

	return gui;
}

GuiPtr GuiManager::loadGui(const std::string& guiPath)
{
	ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(guiPath);

	if (file == NULL) return GuiPtr();

	// Construct a Code Tokeniser, which is able to handle #includes
	try
	{
		std::string whiteSpace = std::string(parser::WHITESPACE) + ",";
		parser::CodeTokeniser tokeniser(file, whiteSpace.c_str(), "{}(),");

		return Gui::createFromTokens(tokeniser);
	}
	catch (parser::ParseException& p)
	{
		globalErrorStream() << "Error while parsing " << guiPath << ": "
			<< p.what() << std::endl;

		return GuiPtr();
	}
}

GuiManager& GuiManager::Instance()
{
	static GuiManager _instance;
	return _instance;
}

} // namespace gui
