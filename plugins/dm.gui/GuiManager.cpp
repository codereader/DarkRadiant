#include "GuiManager.h"

namespace gui
{

GuiPtr GuiManager::getGui(const std::string& guiPath)
{
	// TODO
	return GuiPtr();
}

GuiManager& GuiManager::Instance()
{
	static GuiManager _instance;
	return _instance;
}

} // namespace gui
