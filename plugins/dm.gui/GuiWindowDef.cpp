#include "GuiWindowDef.h"

#include "parser/DefTokeniser.h"
#include "string/string.h"
#include "itextstream.h"

namespace gui
{

Vector4 GuiWindowDef::parseVector4(parser::DefTokeniser& tokeniser)
{
	// Collect tokens until all four components are parsed
	std::vector<std::string> comp;

	while (comp.size() < 4 && tokeniser.hasMoreTokens())
	{
		std::string token = tokeniser.nextToken();

		if (token == ",") continue;

		// TODO: Catch GUI expressions

		comp.push_back(token);
	}

	if (comp.size() != 4)
	{
		throw parser::ParseException("Couldn't parse Vector4, not enough components found.");
	}

	return Vector4(strToDouble(comp[0]), strToDouble(comp[1]), strToDouble(comp[2]), strToDouble(comp[3]));
}

float GuiWindowDef::parseFloat(parser::DefTokeniser& tokeniser)
{
	// TODO: Catch GUI expressions

	return strToFloat(tokeniser.nextToken());
}

std::string GuiWindowDef::parseString(parser::DefTokeniser& tokeniser)
{
	// TODO: Catch GUI expressions

	return tokeniser.nextToken();
}

bool GuiWindowDef::parseBool(parser::DefTokeniser& tokeniser)
{
	// TODO: Catch GUI expressions

	return strToInt(tokeniser.nextToken()) != 0;
}

void GuiWindowDef::addWindow(const GuiWindowDefPtr& window)
{
	_children.push_back(window);
}

void GuiWindowDef::constructFromTokens(parser::DefTokeniser& tokeniser)
{
	// The windowDef keyword has already been parsed, so expect a name plus an opening brace here
	name = tokeniser.nextToken();

	tokeniser.assertNextToken("{");

	while (tokeniser.hasMoreTokens())
	{
		std::string token = tokeniser.nextToken();

		if (token == "rect")
		{
			rect = parseVector4(tokeniser);
		}
		else if (token == "visible")
		{
			visible = parseBool(tokeniser);
		}
		else if (token == "forecolor")
		{
			forecolor = parseVector4(tokeniser);
		}
		else if (token == "backcolor")
		{
			backcolor = parseVector4(tokeniser);
		}
		else if (token == "bordercolor")
		{
			bordercolor = parseVector4(tokeniser);
		}
		else if (token == "matcolor")
		{
			matcolor = parseVector4(tokeniser);
		}
		else if (token == "rotate")
		{
			rotate = parseFloat(tokeniser);
		}
		else if (token == "text")
		{
			text = parseString(tokeniser);
		}
		else if (token == "background")
		{
			background = parseString(tokeniser);
		}
		else if (token == "noevents")
		{
			noevents = parseBool(tokeniser);
		}
		else if (token == "menugui")
		{
			menugui = parseBool(tokeniser);
		}
		else if (token == "windowDef")
		{
			// Child windowdef
			GuiWindowDefPtr window(new GuiWindowDef);
			window->constructFromTokens(tokeniser);

			addWindow(window);
		}
		else if (token == "}")
		{
			break;
		}
		else
		{
			globalWarningStream() << "Unknown token encountered in GUI: " << token << std::endl;
		}
	}
}

}
