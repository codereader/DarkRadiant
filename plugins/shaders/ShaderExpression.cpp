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

	typedef std::stack<IShaderExpressionPtr> OperandStack;
	typedef std::stack<BinaryExpressionPtr> OperatorStack;

public:
	ShaderExpressionParser(parser::DefTokeniser& tokeniser) :
		_tokeniser(tokeniser)
	{}

	IShaderExpressionPtr getExpression()
	{
		// The local variable and operator stack
		OperandStack operands;
		OperatorStack operators;

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
				operands.push(term);
				continue;
			}
			
			// Not a term, do we have an operator at hand?

			// Get an operator
			BinaryExpressionPtr op = getOperatorForToken(token);

			if (op)
			{
				if (operands.empty())
				{
					throw parser::ParseException("Missing operand for operator: " + token);
				}

				// Check precedence if we have previous operators
				if (!operators.empty())
				{
					if (operators.top()->getPrecedence() <= op->getPrecedence())
					{
						finaliseOperator(operands, operators);
					}
				}

				// Push this one on the operator stack
				operators.push(op);
			}
			else
			{
				// Not an operand, not an operator, we seem to be done here
				// TODO: We've exhausted the tokeniser too much by one token!
				break;
			}
		}

		// Roll up the operations
		while (!operators.empty())
		{
			finaliseOperator(operands, operators);
		}

		if (operands.empty())
		{
			throw parser::ParseException("Missing expression");
		}
		
		IShaderExpressionPtr rv = operands.top();
		operands.pop();

		assert(operands.empty()); // there should be nothing left on the stack

		return rv;
	}

private:
	void finaliseOperator(OperandStack& operands, OperatorStack& operators)
	{
		// Need two operands for an operator
		if (operands.size() < 2)
		{
			throw parser::ParseException("Too few operands for operator.");
		}

		const BinaryExpressionPtr& op = operators.top();

		// Set operand B first, we're dealing with a LIFO stack
		op->setB(operands.top());
		operands.pop();

		op->setA(operands.top());
		operands.pop();

		// Push the result back on the stack, as operand
		operands.push(op);

		// Remove the operator, after we've copied it to the operand stack
		operators.pop();
	}

	IShaderExpressionPtr getTerm(const std::string& token)
	{
		if (boost::algorithm::istarts_with(token, "parm"))
		{
			// This is a shaderparm, get the number
			int shaderParmNum = strToInt(token.substr(4));
		
			if (shaderParmNum >= 0 && shaderParmNum <= MAX_SHADERPARM_INDEX)
			{
				return IShaderExpressionPtr(new ShaderParmExpression(shaderParmNum));
			}
			else
			{
				throw new parser::ParseException("Shaderparm index out of bounds");
			}
		}
		else if (token == "time")
		{
			return IShaderExpressionPtr(new TimeExpression);
		}
		else if (token == "sound")
		{
			// No sound support so far
			return IShaderExpressionPtr(new ConstantExpression(0));
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
				return IShaderExpressionPtr(new TableLookupExpression(table, lookupValue));
			}
			else
			{
				// Attempt to convert the token into a floating point value
				try
				{
					float value = boost::lexical_cast<float>(token);
					return IShaderExpressionPtr(new ConstantExpression(value));
				}
				catch (boost::bad_lexical_cast&)
				{}
			}
		}

		return IShaderExpressionPtr();
	}

	// Helper routines
	BinaryExpressionPtr getOperatorForToken(const std::string& token)
	{
		if (token == "+")
		{
			return BinaryExpressionPtr(new AddExpression);
		}
		else if (token == "-")
		{
			return BinaryExpressionPtr(new SubtractExpression);
		}
		else if (token == "*")
		{
			return BinaryExpressionPtr(new MultiplyExpression);
		}
		else if (token == "/")
		{
			return BinaryExpressionPtr(new DivideExpression);
		}
		else if (token == "%")
		{
			return BinaryExpressionPtr(new ModuloExpression);
		}
		else if (token == "<")
		{
			return BinaryExpressionPtr(new LesserThanExpression);
		}
		else if (token == "<=")
		{
			return BinaryExpressionPtr(new LesserThanOrEqualExpression);
		}
		else if (token == ">")
		{
			return BinaryExpressionPtr(new GreaterThanExpression);
		}
		else if (token == ">=")
		{
			return BinaryExpressionPtr(new GreaterThanOrEqualExpression);
		}
		else if (token == "==")
		{
			return BinaryExpressionPtr(new EqualityExpression);
		}
		else if (token == "!=")
		{
			return BinaryExpressionPtr(new InequalityExpression);
		}
		else if (token == "&&")
		{
			return BinaryExpressionPtr(new LogicalAndExpression);
		}
		else if (token == "||")
		{
			return BinaryExpressionPtr(new LogicalOrExpression);
		}

		return BinaryExpressionPtr();
	}
};

} // namespace expressions

IShaderExpressionPtr ShaderExpression::createFromTokens(parser::DefTokeniser& tokeniser)
{
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

IShaderExpressionPtr ShaderExpression::createFromString(const std::string& exprStr)
{
	parser::BasicDefTokeniser<std::string> tokeniser(exprStr, parser::WHITESPACE, "()[]-+*/%");
	return createFromTokens(tokeniser);
}

} // namespace
