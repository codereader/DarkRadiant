#include "GuiScript.h"

#include "itextstream.h"
#include "parser/DefTokeniser.h"
#include "Gui.h"
#include "GuiExpression.h"
#include "Variable.h"

#include "string/case_conv.h"
#include "string/trim.h"
#include "string/predicate.h"

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

	// Any opening and closing parentheses are handled by the expression parser
	ifStatement->_condition = getIfExpression(tokeniser); // condition

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

	st->args.push_back(_owner.parseString(tokeniser)); // variable

	// Add all tokens up to the semicolon as arguments
	while (true)
	{
		std::string token = tokeniser.peek();

		// Sometimes the semicolon is missing
		if (token == ";" || token == "}") break;

		st->args.push_back(ConstantExpression<std::string>::Create(tokeniser.nextToken())); // argument
	}

	pushStatement(st);
}

void GuiScript::parseTransitionStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: transition [window::]<variable> <from> <to> <time> [ <accel> <decel> ]
	StatementPtr st(new Statement(Statement::ST_TRANSITION));

	st->args.push_back(ConstantExpression<std::string>::Create(tokeniser.nextToken())); // variable

	st->args.push_back(ConstantExpression<std::string>::Create(tokeniser.nextToken())); // from
	st->args.push_back(ConstantExpression<std::string>::Create(tokeniser.nextToken())); // to
	st->args.push_back(ConstantExpression<std::string>::Create(tokeniser.nextToken())); // time

	if (tokeniser.peek() != ";")
	{
		// no semicolon, parse optional acceleration and deceleration
		st->args.push_back(ConstantExpression<std::string>::Create(tokeniser.nextToken())); 	// accel
		st->args.push_back(ConstantExpression<std::string>::Create(tokeniser.nextToken()));	// decel

		tokeniser.assertNextToken(";");
	}
	else
	{
		tokeniser.nextToken(); // pull semicolon
	}

	pushStatement(st);
}

void GuiScript::parseSetFocusStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: setFocus <window>
	StatementPtr st(new Statement(Statement::ST_SET_FOCUS));

	st->args.push_back(_owner.parseString(tokeniser)); // window
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

	std::string token = tokeniser.peek();

	if (token != ";")
	{
		// Check if this is a numeric token
		try
        {
			// Remove quotes from string before checking numerics
			std::string trimmed = string::trim_copy(token, "\"");

			// Try to cast the string to a number, throws
			std::stoul(trimmed);

			// Cast succeeded this is just the time for the current window
			st->args.push_back(_owner.parseString(tokeniser));

			tokeniser.assertNextToken(";");
        }
		catch (std::logic_error&)
        {
            // Not a number, must be window plus time
			st->args.push_back(_owner.parseString(tokeniser)); // window
			st->args.push_back(_owner.parseString(tokeniser)); // time
			tokeniser.assertNextToken(";");
        }
	}

	pushStatement(st);
}

void GuiScript::parseShowCursorStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: showCursor <bool>
	StatementPtr st(new Statement(Statement::ST_SHOW_CURSOR));

	st->args.push_back(_owner.parseString(tokeniser)); // boolean
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

	st->args.push_back(_owner.parseString(tokeniser)); // sound
	tokeniser.assertNextToken(";");

	pushStatement(st);
}

void GuiScript::parseRunScriptStatement(parser::DefTokeniser& tokeniser)
{
	// Prototype: runScript <function>
	StatementPtr st(new Statement(Statement::ST_RUNSCRIPT));

	st->args.push_back(_owner.parseString(tokeniser)); // function
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
	else if (token == "{")
	{
		std::size_t blockLevel = ++_curLevel;

		// Another block, a group of statements, enter recursion
		while (tokeniser.hasMoreTokens() && _curLevel == blockLevel)
		{
			std::string nextToken = tokeniser.nextToken();
			string::to_lower(nextToken);

			switchOnToken(nextToken, tokeniser);
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
		rWarning() << "Unknown token " << token << " in GUI script in "
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
	string::to_lower(token);

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
	return _statements.size();
}

std::size_t GuiScript::pushStatement(const StatementPtr& statement)
{
	_statements.push_back(statement);

	return _statements.size() - 1;
}

GuiExpressionPtr GuiScript::getExpression(parser::DefTokeniser& tokeniser)
{
	return GuiExpression::createFromTokens(_owner.getGui(), tokeniser);
}

GuiExpressionPtr GuiScript::getIfExpression(parser::DefTokeniser& tokeniser)
{
	return getExpression(tokeniser);
}

const Statement& GuiScript::getStatement(std::size_t index)
{
	assert(index < _statements.size());
	return *_statements[index];
}

VariablePtr GuiScript::getVariableFromExpression(const std::shared_ptr<IGuiExpression<std::string>>& expression)
{
	std::string expr = expression->evaluate();

	// Not a gui:: variable, check if a namespace has been specified
	std::size_t ddPos = expr.find("::");

	if (ddPos != std::string::npos)
	{
		// Retrieve the windowDef name
		std::string windowDefName = expr.substr(0, ddPos);

		if (windowDefName == "gui")
		{
			// Is a GUI state variable
			return std::make_shared<GuiStateVariable>(_owner.getGui(), expr.substr(ddPos + 2));
		}

		// Look up the windowDef
		IGuiWindowDefPtr windowDef = _owner.getGui().findWindowDef(windowDefName);

		if (windowDef)
		{
			// Cut off the "<windowDef>::" from the name
			return std::make_shared<AssignableWindowVariable>(*windowDef, expr.substr(ddPos+2));
		}
		else
		{
			rWarning() << "GUI Script: unknown windowDef " << windowDefName << std::endl;
			return VariablePtr();
		}
	}
	else
	{
		// Use the owner windowDef if no namespace was defined
		return std::make_shared<AssignableWindowVariable>(_owner, expr);
	}
}

std::string GuiScript::getValueFromExpression(const std::shared_ptr<IGuiExpression<std::string>>& expr)
{
	std::string value = expr->evaluate();

	if (string::starts_with(value, "$gui::"))
	{
		// This is the value of a GUI state variable
		return _owner.getGui().getStateString(value.substr(6));
	}

	return value;
}

void GuiScript::execute()
{
	// Go back to the beginning
	_ip = 0;

	while (_ip < _statements.size())
	{
		const Statement& st = getStatement(_ip++);

		switch (st.type)
		{
		case Statement::ST_NOP:
			break;
		case Statement::ST_JMP:
			_ip = st.jmpDest;
			break;
		case Statement::ST_SET:
			if (st.args.size() == 2)
			{
				// Try to find the target variable
				VariablePtr var = getVariableFromExpression(st.args[0]);

				if (!var)
				{
					rWarning() << "Cannot assign to variable " << st.args[0] << std::endl;
					continue;
				}

				std::string value = getValueFromExpression(st.args[1]);

				if (!var->assignValueFromString(value))
				{
					rWarning() << "Cannot assign value " << st.args[1] << " to variable " << st.args[1] << std::endl;
				}
			}
			break;
		case Statement::ST_TRANSITION:
			break;
		case Statement::ST_IF:
			// TODO: Evaluate expression, for now just perform the jump
			_ip = st.jmpDest;
			break;
		case Statement::ST_SET_FOCUS:
			break;
		case Statement::ST_ENDGAME:
			break;
		case Statement::ST_RESET_TIME:
			break;
		case Statement::ST_SHOW_CURSOR:
			break;
		case Statement::ST_RESET_CINEMATICS:
			break;
		case Statement::ST_LOCALSOUND:
			break;
		case Statement::ST_RUNSCRIPT:
			break;
		case Statement::ST_EVALREGS:
			break;
		};
	}
}

} // namespace
