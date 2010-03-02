#include "GuiScript.h"

#include "itextstream.h"
#include "parser/DefTokeniser.h"
#include "GuiWindowDef.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>

namespace gui
{

GuiScript::GuiScript(GuiWindowDef& owner) :
	_owner(owner),
	_ip(0),
	_curLevel(0)
{}

void GuiScript::parseIfStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: if (<condition>) <statement> [else <statement>]
	// The initial "if" has already been parsed
	StatementPtr ifStatement(new Statement(Statement::ST_IF));

	tokeniser.assertNextToken("(");
	ifStatement->args.push_back(getExpression(tokeniser)); // condition
	tokeniser.assertNextToken(")");

	// Add the statement at the current position
	pushStatement(ifStatement);

	// Parse the statement(s) to execute if the above condition is true
	parseStatement(tokeniser);

	// Check the next token to see where we need to jump to if the condition evaluates to false
	std::string nextToken = tokeniser.nextToken();

	if (nextToken == "else")
	{
		// There is an "else" block, so we need to add a JMP statement before proceeding
		StatementPtr jmpStatement(new Statement(Statement::ST_JMP));
		pushStatement(jmpStatement);

		// Set the original IF jump position to this else
		ifStatement->jmpDest = getCurPosition();

		// As next step, parse the code in the else block
		parseStatement(tokeniser);

		// Finally, position the jump at the location right after the else block
		jmpStatement->jmpDest = getCurPosition();
	}
	else
	{
		// No else, execution falls through, but we need to set the jump destination first
		ifStatement->jmpDest = getCurPosition();

		switchOnToken(nextToken, tokeniser);
	}
}

void GuiScript::parseSetStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: set [window::]<variable> <value>
	StatementPtr st(new Statement(Statement::ST_SET));

	st->args.push_back(getExpression(tokeniser));
	st->args.push_back(getExpression(tokeniser));

	tokeniser.assertNextToken(";");

	pushStatement(st);
}

void GuiScript::parseTransitionStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: transition [window::]<variable> <from> <to> <time> [ <accel> <decel> ]
	StatementPtr st(new Statement(Statement::ST_TRANSITION));

	st->args.push_back(getExpression(tokeniser)); // variable
	st->args.push_back(getExpression(tokeniser)); // from
	st->args.push_back(getExpression(tokeniser)); // to
	st->args.push_back(getExpression(tokeniser)); // time

	std::string token = tokeniser.nextToken();

	if (token != ";")
	{
		// no semicolon, parse optional acceleration and deceleration
		st->args.push_back(token);						// accel
		st->args.push_back(getExpression(tokeniser));	// decel

		tokeniser.assertNextToken(";");
	}

	pushStatement(st);
}

void GuiScript::parseSetFocusStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: setFocus <window>
	StatementPtr st(new Statement(Statement::ST_SET_FOCUS));

	st->args.push_back(getExpression(tokeniser)); // window
	tokeniser.assertNextToken(";");

	pushStatement(st);
}

void GuiScript::parseEndGameStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: endGame
	StatementPtr st(new Statement(Statement::ST_ENDGAME));
	tokeniser.assertNextToken(";");

	pushStatement(st);
}

void GuiScript::parseResetTimeStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: resetTime [<window>] [<time>]
	StatementPtr st(new Statement(Statement::ST_RESET_TIME));

	std::string token = tokeniser.nextToken();

	if (token != ";")
	{
		// Check if this is a numeric token
		try
        {
			// Try to cast the string to a number, throws
			boost::lexical_cast<std::size_t>(token);

			// Cast succeeded this is just the time for the current window
			st->args.push_back(token);

			tokeniser.assertNextToken(";");
        }
		catch (boost::bad_lexical_cast&)
        {
            // Not a number, must be window plus time
			st->args.push_back(token); // window
			st->args.push_back(getExpression(tokeniser)); // time
			tokeniser.assertNextToken(";");
        }
	}

	pushStatement(st);
}

void GuiScript::parseShowCursorStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: showCursor <bool>
	StatementPtr st(new Statement(Statement::ST_SHOW_CURSOR));

	st->args.push_back(getExpression(tokeniser)); // boolean
	tokeniser.assertNextToken(";");

	pushStatement(st);
}

void GuiScript::parseResetCinematicStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: resetCinematics
	StatementPtr st(new Statement(Statement::ST_RESET_CINEMATICS));
	tokeniser.assertNextToken(";");

	pushStatement(st);
}

void GuiScript::parseLocalSoundStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: localSound <sound>
	StatementPtr st(new Statement(Statement::ST_LOCALSOUND));

	st->args.push_back(getExpression(tokeniser)); // sound
	tokeniser.assertNextToken(";");

	pushStatement(st);
}

void GuiScript::parseRunScriptStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: runScript <function>
	StatementPtr st(new Statement(Statement::ST_RUNSCRIPT));

	st->args.push_back(getExpression(tokeniser)); // function
	tokeniser.assertNextToken(";");

	pushStatement(st);
}

void GuiScript::parseEvalRegsStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: evalRegs
	StatementPtr st(new Statement(Statement::ST_EVALREGS));
	tokeniser.assertNextToken(";");

	pushStatement(st);
}

void GuiScript::switchOnToken(const std::string& token, parser::DefTokeniser& tokeniser)
{
	if (token == "}")
	{
		assert(_curLevel > 0);
		_curLevel--;
	}
	if (token == "{")
	{
		std::size_t blockLevel = ++_curLevel;

		// Another block, a group of statements, enter recursion
		while (tokeniser.hasMoreTokens() && _curLevel == blockLevel)
		{
			std::string token = tokeniser.nextToken();
			boost::algorithm::to_lower(token);

			switchOnToken(token, tokeniser);
		}
	}
	else if (token == "set")
	{
		parseSetStatement(tokeniser);
	}
	else if (token == "transition")
	{
		parseTransitionStatement(tokeniser);
	}
	else if (token == "if")
	{
		parseIfStatement(tokeniser);
	}
	else if (token == "setfocus")
	{
		parseSetFocusStatement(tokeniser);
	}
	else if (token == "endgame")
	{
		parseEndGameStatement(tokeniser);
	}
	else if (token == "resettime")
	{
		parseResetTimeStatement(tokeniser);
	}
	else if (token == "resetcinematics")
	{
		parseResetCinematicStatement(tokeniser);
	}
	else if (token == "showcursor")
	{
		parseShowCursorStatement(tokeniser);
	}
	else if (token == "localsound")
	{
		parseLocalSoundStatement(tokeniser);
	}
	else if (token == "runscript")
	{
		parseRunScriptStatement(tokeniser);
	}
	else if (token == "evalregs")
	{
		parseEvalRegsStatement(tokeniser);
	}
	else if (token == ";")
	{
		// A single semicolon is also a valid statement, do nothing
	}
	else 
	{
		globalWarningStream() << "Unknown token " << token << " in GUI script in "
			<< _owner.name << std::endl;
	}
}

void GuiScript::parseStatement(parser::DefTokeniser& tokeniser)
{
	if (!tokeniser.hasMoreTokens())
	{
		return;
	}

	std::string token = tokeniser.nextToken();
	boost::algorithm::to_lower(token);

	switchOnToken(token, tokeniser);
}

void GuiScript::constructFromTokens(parser::DefTokeniser& tokeniser)
{
	// Remove any previous statements
	_statements.clear();
	_ip = 0;

	// Treat the upcoming { } block as "Statement"
	parseStatement(tokeniser);
}

std::size_t GuiScript::getCurPosition()
{
	return _statements.size() - 1;
}

std::size_t GuiScript::pushStatement(const StatementPtr& statement)
{
	_statements.push_back(statement);

	return _statements.size() - 1;
}

std::string GuiScript::getExpression(parser::DefTokeniser& tokeniser)
{
	return tokeniser.nextToken();
}

} // namespace
