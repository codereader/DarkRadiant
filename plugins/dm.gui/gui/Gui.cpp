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

void Gui::initTime(const std::size_t time)
{
	if (_desktop != NULL)
	{
		_desktop->initTime(time, true);
	}
}

void Gui::update(const std::size_t timestep)
{
	if (_desktop != NULL)
	{
		// Recursively update child windowDefs
		_desktop->update(timestep, true);
	}
}

GuiWindowDefPtr Gui::findWindowDef(const std::string& name)
{
	// Handle "Desktop" right here
	if (name == "Desktop")
	{
		return _desktop;
	}

	return (_desktop != NULL) ? _desktop->findWindowDef(name) : GuiWindowDefPtr();
}

} // namespace
