#include "Gui.h"

namespace gui
{

Gui::Gui()
{
}

void Gui::addWindow(const GuiWindowDefPtr& window)
{
	_windows.push_back(window);
}

GuiPtr Gui::createFromTokens(parser::DefTokeniser& tokeniser)
{
	GuiPtr gui(new Gui);

	while (tokeniser.hasMoreTokens())
	{
		std::string token = tokeniser.nextToken();

		if (token == "windowDef")
		{
			// Construct a new window and add it to the list
			GuiWindowDefPtr window(new GuiWindowDef);
			window->constructFromTokens(tokeniser);

			gui->addWindow(window);
		}
	}

	return gui;
}

} // namespace
