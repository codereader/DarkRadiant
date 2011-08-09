#include "ShaderExpression.h"

#include "itextstream.h"

#include "string/string.h"
#include <boost/algorithm/string/predicate.hpp>
#include "Doom3ShaderSystem.h"

#include <stack>

namespace shaders
{

namespace expressions
{

namespace
{
	const int MAX_SHADERPARM_INDEX = 11;
}

class ShaderExpressionParser
{
private:
	parser::DefTokeniser& _tokeniser;

	// Two stacks, one for operands, one for operators
	std::stack<IShaderExpressionPtr> _operands;
	std::stack<BinaryExpressionPtr> _operators;

public:
	ShaderExpressionParser(parser::DefTokeniser& tokeniser) :
		_tokeniser(tokeniser)
	{}

	IShaderExpressionPtr getExpression()
	{
		while (_tokeniser.hasMoreTokens())
		{
			std::string token = _tokeniser.nextToken();

			// Get a new term, push it on the stack
			IShaderExpressionPtr term;

			if (token == "(")
			{
				// New scope, treat this as new expression
				term = getExpression();
			}
			else if (token == ")" || token == "]")
			{
				// End of scope reached, break the loop and roll up the expression
				break;
			}
			else
			{
				// No parantheses, get a new term, push it on the stack
				term = getTerm(token);
			}			

			if (term)
			{
				_operands.push(term);
				continue;
			}
			
			// Not a term, do we have an operator at hand?

			// Get an operator
			BinaryExpressionPtr op = getOperatorForToken(token);

			if (op)
			{
				if (_operands.empty())
				{
					throw parser::ParseException("Missing operand for operator: " + token);
				}

				// Push this one on the operator stack
				_operators.push(op);
			}
		}

		// Roll up the operations
		while (!_operators.empty())
		{
			// Need two operands for an operator
			if (_operands.size() < 2)
			{
				throw parser::ParseException("Too few operands for operator.");
			}

			const BinaryExpressionPtr& op = _operators.top();

			op->setA(_operands.top());
			_operands.pop();

			op->setB(_operands.top());
			_operands.pop();

			// Push the result back on the stack, as operand
			_operands.push(op);

			_operators.pop();
		}

		if (_operands.empty())
		{
			throw parser::ParseException("Missing expression");
		}
		
		IShaderExpressionPtr rv = _operands.top();
		_operands.pop();

		assert(_operands.empty()); // there should be nothing left on the stack

		return rv;
	}

private:
	IShaderExpressionPtr getTerm(const std::string& token)
	{
		IShaderExpressionPtr rv;

		if (boost::algorithm::istarts_with(token, "parm"))
		{
			// This is a shaderparm, get the number
			int shaderParmNum = strToInt(token.substr(4));
		
			if (shaderParmNum >= 0 && shaderParmNum <= MAX_SHADERPARM_INDEX)
			{
				rv.reset(new ShaderParmExpression(shaderParmNum));
			}
			else
			{
				throw new parser::ParseException("Shaderparm index out of bounds");
			}
		}
		else if (token == "time")
		{
			rv.reset(new TimeExpression);
		}
		else if (token == "sound")
		{
			// No sound support so far
			rv.reset(new ConstantExpression(0));
		}
		else 
		{
			// Check if this keyword is a material lookup table
			TableDefinitionPtr table = GetShaderSystem()->getTableForName(token);

			if (table != NULL)
			{
				// Got it, this is a table name
				// We need an opening '[' to have valid syntax
				_tokeniser.assertNextToken("[");

				// The lookup expression itself has to be parsed afresh, enter recursion
				IShaderExpressionPtr lookupValue = getExpression();

				if (lookupValue == NULL)
				{
					throw new parser::ParseException("Missing or invalid expression in table lookup operator[]");
				}

				// Construct a new table lookup expression and link them together
				rv.reset(new TableLookupExpression(table, lookupValue));
			}
			else
			{
				// Attempt to convert the token into a floating point value
				try
				{
					float value = boost::lexical_cast<float>(token);
					rv.reset(new ConstantExpression(value));
				}
				catch (boost::bad_lexical_cast&)
				{}
			}
		}

		return rv;
	}

	// Helper routines
	BinaryExpressionPtr getOperatorForToken(const std::string& token)
	{
		BinaryExpressionPtr rv;

		if (token == "+")
		{
			rv.reset(new AddExpression);
		}

		return rv;
	}
};

#if 0
IShaderExpressionPtr ShaderExpression::createFromTokens(parser::StringTokeniser& tokeniser)
{
	std::string token = tokeniser.nextToken();

	// The first expression (must not be an operator)
	IShaderExpressionPtr first;

	if (token == "(")
	{
		// A paranthesis always suggests a new token, enter recursion to retrieve the first
		first = createFromTokens(tokeniser);
	}
	else if (boost::algorithm::istarts_with(token, "parm"))
	{
		// This is a shaderparm, get the number
		int shaderParmNum = strToInt(token.substr(4));
		
		if (shaderParmNum >= 0 && shaderParmNum <= MAX_SHADERPARM_INDEX)
		{
			first.reset(new ShaderParmExpression(shaderParmNum));
		}
		else
		{
			throw new parser::ParseException("Shaderparm index out of bounds");
		}
	}
	else if (token == "time")
	{
		first.reset(new TimeExpression);
	}
	else 
	{
		// Check if this keyword is a material lookup table
		TableDefinitionPtr table = GetShaderSystem()->getTableForName(token);

		if (table != NULL)
		{
			// Got it, this is a table name
			// We need an opening '[' to have valid syntax
			tokeniser.assertNextToken("[");

			// The lookup expression itself has to be parsed afresh, enter recursion
			IShaderExpressionPtr lookupValue = createFromTokens(tokeniser);

			if (lookupValue == NULL)
			{
				throw new parser::ParseException("Missing or invalid expression in table lookup operator[]");
			}

			// Construct a new table lookup expression and link them together
			first.reset(new TableLookupExpression(table, lookupValue));
		}
		else
		{
			// Attempt to convert the token into a floating point value
			try
			{
				float value = boost::lexical_cast<float>(token);
				first.reset(new ConstantExpression(value));
			}
			catch (boost::bad_lexical_cast& ex)
			{
				throw new parser::ParseException(
					"Expected time, parm<N> , table name or floating point value, found: " +
					token + " (" + ex.what() + ")");
			}
		}
	}

	// The potential operator and second operand
	BinaryExpressionPtr op;
	IShaderExpressionPtr second;

	// The first expression has been parsed and is non-NULL, are there any more tokens?
	while (tokeniser.hasMoreTokens())
	{
		token = tokeniser.nextToken();

		// Check if this is an operator
		BinaryExpressionPtr candidate = getOperatorForToken(token);

		if (candidate)
		{
			if (op && !second)
			{
				// Syntax error
				throw parser::ParseException("Operand expected.");
			}

#if 0
			// Close any pending operators
			if (op && second)
			{
				// We already have an operator in our stack, check which one takes precedence
				if (candidate->getPrecedence() > op->getPrecedence())
				{

				}
			}

			op = candidate;

			// Link the first operand to this operator
			op->setA(first);
#endif
		}
		else if (token == "]")
		{
			// End of table lookup - break the loop and return
			break;
		}
	}

	// If we don't have an operator at hand this is a "standalone" expression
	if (!op)
	{
		return first;
	}

	return IShaderExpressionPtr();
}
#endif

} // namespace expressions

IShaderExpressionPtr ShaderExpression::createFromString(const std::string& exprStr)
{
	parser::BasicDefTokeniser<std::string> tokeniser(exprStr, parser::WHITESPACE, "()[]-+*/%|&");

	try
	{
		expressions::ShaderExpressionParser parser(tokeniser);

		return parser.getExpression();
	}
	catch (parser::ParseException& ex)
	{
		globalWarningStream() << "[shaders] " << ex.what() << std::endl;
		return IShaderExpressionPtr();
	}
}

} // namespace
