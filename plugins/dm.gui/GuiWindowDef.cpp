#include "GuiWindowDef.h"

#include "parser/DefTokeniser.h"
#include "string/string.h"
#include "itextstream.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/case_conv.hpp>

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

		if (token.find(',') != std::string::npos)
		{
			std::vector<std::string> parts;
			boost::algorithm::split(parts, token, boost::algorithm::is_any_of(","));

			for (std::size_t i = 0; i < parts.size(); ++i)
			{
				comp.push_back(boost::algorithm::trim_copy(parts[i]));
			}

			continue;
		}

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

int GuiWindowDef::parseInt(parser::DefTokeniser& tokeniser)
{
	// TODO: Catch GUI expressions

	return strToInt(tokeniser.nextToken());
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
		boost::algorithm::to_lower(token);

		if (token == "rect")
		{
			rect = parseVector4(tokeniser);
		}
		else if (token == "visible")
		{
			visible = parseBool(tokeniser);
		}
		else if (token == "notime")
		{
			notime = parseBool(tokeniser);
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
		else if (token == "font")
		{
			font = tokeniser.nextToken();
		}
		else if (token == "textscale")
		{
			textscale = parseFloat(tokeniser);
		}
		else if (token == "textalign")
		{
			textalign = parseInt(tokeniser);
		}
		else if (token == "background")
		{
			background = parseString(tokeniser);
		}
		else if (token == "noevents")
		{
			noevents = parseBool(tokeniser);
		}
		else if (token == "modal")
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
		else if (token == "onTime")
		{
			// TODO
			std::string time = tokeniser.nextToken();
			tokeniser.assertNextToken("{");

			std::size_t depth = 1;

			while (tokeniser.hasMoreTokens() && depth > 0)
			{
				std::string token = tokeniser.nextToken();
				if (token == "}") depth--;
				if (token == "{") depth++;
			}
		}
		else if (token == "onNamedEvent")
		{
			// TODO
			std::string eventName = tokeniser.nextToken();
			tokeniser.assertNextToken("{");

			std::size_t depth = 1;

			while (tokeniser.hasMoreTokens() && depth > 0)
			{
				std::string token = tokeniser.nextToken();
				if (token == "}") depth--;
				if (token == "{") depth++;
			}
		}
		else if (token == "onEsc")
		{
			// TODO
			tokeniser.assertNextToken("{");

			std::size_t depth = 1;

			while (tokeniser.hasMoreTokens() && depth > 0)
			{
				std::string token = tokeniser.nextToken();
				if (token == "}") depth--;
				if (token == "{") depth++;
			}
		}
		else if (token == "onMouseEnter" || token == "onMouseExit")
		{
			// TODO
			tokeniser.assertNextToken("{");

			std::size_t depth = 1;

			while (tokeniser.hasMoreTokens() && depth > 0)
			{
				std::string token = tokeniser.nextToken();
				if (token == "}") depth--;
				if (token == "{") depth++;
			}
		}
		else if (token == "onAction")
		{
			// TODO
			tokeniser.assertNextToken("{");

			std::size_t depth = 1;

			while (tokeniser.hasMoreTokens() && depth > 0)
			{
				std::string token = tokeniser.nextToken();
				if (token == "}") depth--;
				if (token == "{") depth++;
			}
		}
		else if (token == "float" || token == "definefloat")
		{
			// TODO: Add variable
			std::string variableName = tokeniser.nextToken();
		}
		else if (token == "definevec4")
		{
			// TODO: Add variable
			std::string variableName = tokeniser.nextToken();
			parseVector4(tokeniser);
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
