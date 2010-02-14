#include "GuiManager.h"

#include "iarchive.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "parser/DefTokeniser.h"
#include <boost/shared_array.hpp>

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
	// Load the complete source of one GUI into memory
	// This is outsourced into its own method, as #include statements need to be resolved.
	// This process could be changed into live parsing if we had a special inputstream taking
	// care of the #includes on the fly (FIXME).
	std::string guiSource = loadGuiSource(guiPath);

	// Couldn't load file, result is empty, return NULL
	if (guiSource.empty()) return GuiPtr();

	parser::BasicDefTokeniser<std::string> tokeniser(guiSource);

	return GuiPtr();
}

GuiManager& GuiManager::Instance()
{
	static GuiManager _instance;
	return _instance;
}

std::string GuiManager::loadGuiSource(const std::string& guiPath)
{
	std::string returnValue;

	returnValue = loadFile(guiPath);

	// TODO: Resolve #include statements or implement smart InputStream

	return returnValue;
}

std::string GuiManager::loadFile(const std::string& vfsPath)
{
	std::string returnValue;

	ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(vfsPath);

	if (file == NULL) return returnValue;

	TextInputStream& stream = file->getInputStream();

	const std::size_t BUF_SIZE = 8192;
	boost::shared_array<char> buffer(new char[BUF_SIZE+1]);

	while (true)
	{
		std::size_t bytesRead = stream.read(buffer.get(), BUF_SIZE);

		if (bytesRead == 0) break;

		// NULL-terminate the string
		buffer[bytesRead] = '\0';

		returnValue += buffer.get();
	}

	return returnValue;
}

} // namespace gui
