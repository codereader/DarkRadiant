#include "GuiManager.h"

#include "iarchive.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "parser/DefTokeniser.h"
#include "stream/DeclFileInputStream.h"

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

	if (gui == NULL)
	{
		globalWarningStream() << "Could not find GUI: " << guiPath << std::endl;
	}

	return gui;
}

GuiPtr GuiManager::loadGui(const std::string& guiPath)
{
	ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(guiPath);

	if (file == NULL) return GuiPtr();

	// Construct a new Declaration File Stream, which is able to handle #includes
	DeclFileInputStream stream(file);
	std::istream is(&stream);

	parser::BasicDefTokeniser<std::istream> tokeniser(is);

	return Gui::createFromTokens(tokeniser);
}

GuiManager& GuiManager::Instance()
{
	static GuiManager _instance;
	return _instance;
}

} // namespace gui
