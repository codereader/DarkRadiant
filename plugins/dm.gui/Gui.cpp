#include "Gui.h"

namespace gui
{

Gui::Gui()
{
}

GuiPtr Gui::createFromTokens(parser::DefTokeniser& tokeniser)
{
	while (tokeniser.hasMoreTokens())
	{
		tokeniser.nextToken();
	}

	return GuiPtr();
}

} // namespace
