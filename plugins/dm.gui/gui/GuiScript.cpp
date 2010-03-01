#include "GuiScript.h"

#include "itextstream.h"
#include "parser/DefTokeniser.h"
#include "GuiWindowDef.h"

#include <boost/algorithm/string/case_conv.hpp>

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
		boost::algorithm::to_lower(token);

		if (token == "}") 
		{
			depth--;
		}
		else if (token == "{") 
		{
			depth++;
		}
		else if (token == "set")
		{
			std::string targetExpr = getExpression(tokeniser);
			std::string valueExpr = getExpression(tokeniser);
			tokeniser.assertNextToken(";");
		}
		else if (token == "transition")
		{
			// TODO
		}
		else if (token == "if")
		{
			// TODO
		}
		else if (token == "else")
		{
			// TODO
		}
		else if (token == "setfocus")
		{
			// TODO
		}
		else if (token == "endgame")
		{
			// TODO
		}
		else if (token == "resettime")
		{
			// TODO
		}
		else if (token == "resetcinematics")
		{
			// TODO
		}
		else if (token == "showcursor")
		{
			// TODO
		}
		else if (token == "localsound")
		{
			// TODO
		}
		else if (token == "runscript")
		{
			// TODO
		}
		else if (token == "evalregs")
		{
			// Nothing
		}
		else 
		{
			globalWarningStream() << "Unknown token " << token << " in GUI script in "
				<< _owner.name << std::endl;
		}
	}
}

std::string GuiScript::getExpression(parser::DefTokeniser& tokeniser)
{
	return tokeniser.nextToken();
}

} // namespace
