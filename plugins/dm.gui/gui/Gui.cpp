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
				rError() << "Cannot define multiple top-level windowDefs" << std::endl;
			}
		}
	}

	return gui;
}

void Gui::setStateString(const std::string& key, const std::string& value)
{
	_state[key] = value;

	// TODO: Handle state variable links to windowDef registers
}

std::string Gui::getStateString(const std::string& key)
{
	GuiState::const_iterator i = _state.find(key);

	return (i != _state.end()) ? i->second : "";
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

void Gui::pepareRendering()
{
	if (_desktop != NULL)
	{
		// Recursively prepare child windowDefs
		_desktop->pepareRendering(true);
	}
}

} // namespace
