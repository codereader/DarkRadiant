#include "GuiScript.h"

#include "parser/DefTokeniser.h"
#include "GuiWindowDef.h"

namespace gui
{

GuiScript::GuiScript(GuiWindowDef& owner) :
	_owner(owner)
{}

void GuiScript::constructFromTokens(parser::DefTokeniser& tokeniser)
{
	tokeniser.assertNextToken("{");

	std::size_t depth = 1;

	while (tokeniser.hasMoreTokens() && depth > 0)
	{
		std::string token = tokeniser.nextToken();

		if (token == "}") 
		{
			depth--;
		}
		else if (token == "{") 
		{
			depth++;
		}
		else
		{
			// TODO
		}
	}
}

} // namespace
