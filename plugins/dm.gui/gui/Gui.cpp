#include "Gui.h"
#include "itextstream.h"

namespace gui
{

const IGuiWindowDefPtr& Gui::getDesktop() const
{
	return _desktop;
}

void Gui::setDesktop(const IGuiWindowDefPtr& newDesktop)
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

	// Handle state variable links to windowDef registers
	GuiStateChangedSignals::iterator i = _stateSignals.find(key);

	if (i != _stateSignals.end())
	{
		i->second.emit();
	}
}

sigc::signal<void>& Gui::getChangedSignalForState(const std::string& key)
{
	GuiStateChangedSignals::iterator i = _stateSignals.find(key);

	if (i != _stateSignals.end())
	{
		return i->second;
	}

	// Insert a new signal
	std::pair<GuiStateChangedSignals::iterator, bool> result =
		_stateSignals.insert(std::make_pair(key, sigc::signal<void>()));

	return result.first->second;
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

IGuiWindowDefPtr Gui::findWindowDef(const std::string& name)
{
	// Handle "Desktop" right here
	if (name == "Desktop")
	{
		return _desktop;
	}

	return (_desktop != NULL) ? _desktop->findWindowDef(name) : IGuiWindowDefPtr();
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
