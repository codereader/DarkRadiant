#include "GuiWindowDef.h"

#include "parser/DefTokeniser.h"
#include "string/convert.h"
#include "itextstream.h"

#include <limits>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "GuiScript.h"

namespace gui
{

GuiWindowDef::GuiWindowDef(Gui& owner) :
	_owner(owner),
	_renderableText(*this),
	_textChanged(true),
	visible(true),
	forecolor(1,1,1,1),
	hovercolor(1,1,1,1),
	backcolor(0,0,0,0),
	bordercolor(0,0,0,0),
	matcolor(1,1,1,1),
	rotate(0),
	textscale(1),
	textalign(0),
	textalignx(0),
	textaligny(0),
	forceaspectwidth(640),
	forceaspectheight(480),
	noclip(false),
	notime(false),
	nocursor(false),
	nowrap(false),
	time(0)
{}

Gui& GuiWindowDef::getGui() const
{
	return _owner;
}

// Returns a GUI expression, which can be a number, a string or a formula ("gui::objVisible" == 1).
std::string GuiWindowDef::getExpression(parser::DefTokeniser& tokeniser)
{
	std::string returnValue = tokeniser.nextToken();

	if (returnValue == "(")
	{
		// Assemble token until closing brace found
		std::size_t depth = 1;

		while (depth > 0 && tokeniser.hasMoreTokens())
		{
			std::string token = tokeniser.nextToken();

			if (token == ")") depth--;

			returnValue += token;
		}
	}

	//  Strip quotes
	boost::algorithm::trim_if(returnValue, boost::algorithm::is_any_of("\""));

	return returnValue;
}

Vector4 GuiWindowDef::parseVector4(parser::DefTokeniser& tokeniser)
{
	// Collect tokens until all four components are parsed
	std::vector<std::string> comp;

	while (comp.size() < 4 && tokeniser.hasMoreTokens())
	{
		std::string token = getExpression(tokeniser);

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

	return Vector4(string::convert<float>(comp[0]),
                   string::convert<float>(comp[1]),
                   string::convert<float>(comp[2]),
                   string::convert<float>(comp[3]));
}

float GuiWindowDef::parseFloat(parser::DefTokeniser& tokeniser)
{
	// TODO: Catch GUI expressions

	return string::convert<float>(getExpression(tokeniser));
}

int GuiWindowDef::parseInt(parser::DefTokeniser& tokeniser)
{
	// TODO: Catch GUI expressions

	return string::convert<int>(getExpression(tokeniser));
}

std::string GuiWindowDef::parseString(parser::DefTokeniser& tokeniser)
{
	// TODO: Catch GUI expressions

	return getExpression(tokeniser);
}

bool GuiWindowDef::parseBool(parser::DefTokeniser& tokeniser)
{
	// TODO: Catch GUI expressions

	return string::convert<int>(getExpression(tokeniser)) != 0;
}

void GuiWindowDef::addWindow(const GuiWindowDefPtr& window)
{
	children.push_back(window);
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
			setText(parseString(tokeniser));
		}
		else if (token == "font")
		{
			font = parseString(tokeniser);

			// Cut off the "fonts/" part
			boost::algorithm::replace_first(font, "fonts/", "");
		}
		else if (token == "textscale")
		{
			textscale = parseFloat(tokeniser);
		}
		else if (token == "textalign")
		{
			textalign = parseInt(tokeniser);
		}
		else if (token == "textalignx")
		{
			textalignx = parseFloat(tokeniser);
		}
		else if (token == "textaligny")
		{
			textaligny = parseFloat(tokeniser);
		}
		else if (token == "forceaspectwidth")
		{
			forceaspectwidth = parseFloat(tokeniser);
		}
		else if (token == "forceaspectheight")
		{
			forceaspectheight = parseFloat(tokeniser);
		}
		else if (token == "background")
		{
			background = parseString(tokeniser);
		}
		else if (token == "noevents")
		{
			noevents = parseBool(tokeniser);
		}
		else if (token == "nocursor")
		{
			nocursor = parseBool(tokeniser);
		}
		else if (token == "noclip")
		{
			noclip = parseBool(tokeniser);
		}
		else if (token == "nowrap")
		{
			nowrap = parseBool(tokeniser);
		}
		else if (token == "modal")
		{
			noevents = parseBool(tokeniser);
		}
		else if (token == "menugui")
		{
			menugui = parseBool(tokeniser);
		}
		else if (token == "windowdef")
		{
			// Child windowdef
			GuiWindowDefPtr window(new GuiWindowDef(_owner));
			window->constructFromTokens(tokeniser);

			addWindow(window);
		}
		else if (token == "ontime")
		{
			std::string timeStr = tokeniser.nextToken();

			// Check the time for validity
			std::size_t time = string::convert<std::size_t>(
                timeStr, std::numeric_limits<std::size_t>::max()
            );

			if (time == std::numeric_limits<std::size_t>::max())
			{
				rWarning() << "Invalid time encountered in onTime event in "
					<< name << ": " << timeStr << std::endl;
			}

			// Allocate a new GuiScript
			GuiScriptPtr script(new GuiScript(*this));

			script->constructFromTokens(tokeniser);

			_timedEvents.insert(TimedEventMap::value_type(time, script));
		}
		else if (token == "onnamedevent")
		{
			std::string eventName = tokeniser.nextToken();

			// Parse the script
			GuiScriptPtr script(new GuiScript(*this));
			script->constructFromTokens(tokeniser);

			// TODO: Save event
		}
		else if (token == "onevent")
		{
			GuiScriptPtr script(new GuiScript(*this));
			script->constructFromTokens(tokeniser);

			// TODO
		}
		else if (token == "onesc")
		{
			GuiScriptPtr script(new GuiScript(*this));
			script->constructFromTokens(tokeniser);

			// TODO
		}
		else if (token == "onmouseenter" || token == "onmouseexit")
		{
			GuiScriptPtr script(new GuiScript(*this));
			script->constructFromTokens(tokeniser);

			// TODO
		}
		else if (token == "onaction")
		{
			GuiScriptPtr script(new GuiScript(*this));
			script->constructFromTokens(tokeniser);

			// TODO
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
			rWarning() << "Unknown token encountered in GUI: " << token << std::endl;
		}
	}
}

const std::string& GuiWindowDef::getText() const
{
	return _text;
}

void GuiWindowDef::setText(const std::string& newText)
{
	_text = newText;
	_textChanged = true;
}

RenderableText& GuiWindowDef::getRenderableText()
{
	if (_textChanged)
	{
		// Text has changed, refresh the renderable
		_textChanged = false;

		// (Re-)compile the renderable
		_renderableText.recompile();
	}

	return _renderableText;
}

void GuiWindowDef::update(const std::size_t timeStep, bool updateChildren)
{
	if (!notime)
	{
		std::size_t oldTime = time;

		// Update this windowDef's time
		time += timeStep;

		// Be sure to include the ontime 0 event the first time
		if (oldTime > 0)
		{
			oldTime++;
		}

		// Check events whose time is within [oldTime..time]
		for (TimedEventMap::const_iterator i = _timedEvents.lower_bound(oldTime);
			 i != _timedEvents.end() && i != _timedEvents.upper_bound(time); ++i)
		{
			i->second->execute();
		}
	}

	// Update children regardless of this windowDef's notime setting
	if (updateChildren)
	{
		for (ChildWindows::const_iterator i = children.begin(); i != children.end(); ++i)
		{
			(*i)->update(timeStep, updateChildren);
		}
	}
}

void GuiWindowDef::initTime(const std::size_t time, bool updateChildren)
{
	this->time = time;

	if (updateChildren)
	{
		for (ChildWindows::const_iterator i = children.begin(); i != children.end(); ++i)
		{
			(*i)->initTime(time, updateChildren);
		}
	}
}

GuiWindowDefPtr GuiWindowDef::findWindowDef(const std::string& name)
{
	// First look at all direct children
	for (ChildWindows::const_iterator i = children.begin(); i != children.end(); ++i)
	{
		if ((*i)->name == name)
		{
			return (*i);
		}
	}

	// Not found, ask each child to search for the windowDef
	for (ChildWindows::const_iterator i = children.begin(); i != children.end(); ++i)
	{
		GuiWindowDefPtr window = (*i)->findWindowDef(name);

		if (window != NULL) return window;
	}

	// Not found
	return GuiWindowDefPtr();
}

void GuiWindowDef::pepareRendering(bool prepareChildren)
{
	// Triggers a re-compilation of the text VBOs, if necessary
	getRenderableText();

	if (!prepareChildren) return;

	for (ChildWindows::const_iterator i = children.begin(); i != children.end(); ++i)
	{
		(*i)->pepareRendering(prepareChildren);
	}
}

} // namespace
