#include "Gui.h"
#include "itextstream.h"

namespace gui
{

Gui::Gui()
{
}

const GuiWindowDefPtr& Gui::getDesktop() const
{
	return _desktop;
}

void Gui::setDesktop(const GuiWindowDefPtr& newDesktop)
{
	_desktop = newDesktop;
}

GuiPtr Gui::createFromTokens(parser::DefTokeniser& tokeniser)
{
	GuiPtr gui(new Gui);

	while (tokeniser.hasMoreTokens())
	{
		std::string token = tokeniser.nextToken();

		if (token == "windowDef")
		{
			if (gui->getDesktop() == NULL)
			{
				GuiWindowDefPtr desktop(new GuiWindowDef(*gui));
				desktop->constructFromTokens(tokeniser);

				gui->setDesktop(desktop);
			}
			else
			{
				globalErrorStream() << "Cannot define multiple top-level windowDefs" << std::endl;
			}
		}
	}

	return gui;
}

} // namespace
