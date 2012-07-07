#include "ShaderExpression.h"

#include "itextstream.h"

#include "string/convert.h"
#include <boost/algorithm/string/predicate.hpp>
#include "Doom3ShaderSystem.h"

#include <stack>
#include <list>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace shaders
{

namespace expressions
{

namespace
{
	const int MAX_SHADERPARM_INDEX = 11;
	const int MAX_GLOBAL_SHADERPARM_INDEX = 7;
}

class ShaderExpressionTokeniser :
	public parser::DefTokeniser
{
private:
	parser::DefTokeniser& _tokeniser;

	// buffer containing tokens pulled from the wrapped tokeniser
	std::list<std::string> _buffer;

	const char* _delims;

public:
	ShaderExpressionTokeniser(parser::DefTokeniser& tokeniser) :
		_tokeniser(tokeniser),
		_delims("[]+-%*/")
	{}

	
    bool hasMoreTokens() const 
	{
		return !_buffer.empty() || _tokeniser.hasMoreTokens();
	}

    std::string nextToken() 
	{
		if (_buffer.empty())
		{
			// Pull a new token from the underlying stream and split it up
			fillBuffer(_tokeniser.nextToken());
		}

		std::string result = _buffer.front();
		_buffer.pop_front();

		return result;
	}

    std::string peek() const 
	{
		if (!_buffer.empty())
		{
			// We have items in the buffer, return that one
			return _buffer.front();
		}
		
		// No items in the buffer, take a peek in the underlying stream
		std::string rawToken = _tokeniser.peek();

		// Split up the token by using a sub-tokeniser, keeping all delimiters
		parser::BasicDefTokeniser<std::string> subtokeniser(rawToken, "", _delims);
		
		// Return the first of these tokens, then exit
		return subtokeniser.nextToken();
	}

private:
	void fillBuffer(const std::string& token)
	{
		// Use a separate tokeniser and keep all delimiters
		parser::BasicDefTokeniser<std::string> subtokeniser(token, "", _delims);

		while (subtokeniser.hasMoreTokens())
		{
			_buffer.push_back(subtokeniser.nextToken());
		}
	}
};

class ShaderExpressionParser
{
private:
	ShaderExpressionTokeniser& _tokeniser;

	typedef std::stack<IShaderExpressionPtr> OperandStack;
	typedef std::stack<BinaryExpressionPtr> OperatorStack;

public:
	ShaderExpressionParser(ShaderExpressionTokeniser& tokeniser) :
		_tokeniser(tokeniser)
	{}

	IShaderExpressionPtr getExpression()
	{
		// The local variable and operator stack
		OperandStack operands;
		OperatorStack operators;

		bool lastTokenWasOperator = false; // to detect signs

		while (_tokeniser.hasMoreTokens())
		{
			// Don't actually pull the token from the tokeniser, we might want to 
			// return the tokeniser to the caller if the token is not part of the expression
			// The token will be exhausted from the stream once it is recognised as keyword
			std::string token = _tokeniser.peek();

			// Get a new term, push it on the stack
			IShaderExpressionPtr term;

			if (token == "(")
			{
				_tokeniser.nextToken(); // valid token, exhaust

				// New scope, treat this as new expression
				term = getExpression();
			}
			else if (token == ")" || token == "]")
			{
				_tokeniser.nextToken(); // valid token, exhaust

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
				// The token has already been pulled from the tokeniser
				operands.push(term);

				lastTokenWasOperator = false;
				continue;
			}
			
			// Not a term, do we have an operator at hand?

			// Get an operator
			BinaryExpressionPtr op = getOperatorForToken(token);

			if (op)
			{
				_tokeniser.nextToken(); // valid token, exhaust

				if (operands.empty() || lastTokenWasOperator)
				{
					lastTokenWasOperator = false; // clear the flag again

					// If this is a + or -, take it as a sign operator
					if (token == "+")
					{
						// A leading +, just ignore it
						continue;
					}
					else if (token == "-")
					{
						// A leading -, interpret it as -1 *
						operands.push(IShaderExpressionPtr(new ConstantExpression(-1)));
						operators.push(BinaryExpressionPtr(new MultiplyExpression));

						// Discard the - operator
						continue;
					}
					else
					{
						throw parser::ParseException("Missing operand for operator: " + token);
					}
				}

				// We have operands, so this is a regular operator
				lastTokenWasOperator = true;

				// Check precedence if we have previous operators
				while (!operators.empty())
				{
					if (operators.top()->getPrecedence() <= op->getPrecedence())
					{
						finaliseOperator(operands, operators);
					}
					else
					{
						break;
					}
				}

				// Push this one on the operator stack
				operators.push(op);
				continue;
			}
			
			// Not an operand, not an operator, we seem to be done here
			break;
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

	// Try to get a valid expression from the token. If the token was found to be valid
	// The token is actually pulled from the tokeniser using nextToken()
	IShaderExpressionPtr getTerm(const std::string& token)
	{
		if (boost::algorithm::istarts_with(token, "parm"))
		{
			_tokeniser.nextToken(); // valid token, exhaust

			// This is a shaderparm, get the number
			int shaderParmNum = string::convert<int>(token.substr(4));
		
			if (shaderParmNum >= 0 && shaderParmNum <= MAX_SHADERPARM_INDEX)
			{
				return IShaderExpressionPtr(new ShaderParmExpression(shaderParmNum));
			}
			else
			{
				throw new parser::ParseException("Shaderparm index out of bounds");
			}
		}
		else if (boost::algorithm::istarts_with(token, "global"))
		{
			_tokeniser.nextToken(); // valid token, exhaust

			// This is a shaderparm, get the number
			int shaderParmNum = string::convert<int>(token.substr(6));
		
			if (shaderParmNum >= 0 && shaderParmNum <= MAX_GLOBAL_SHADERPARM_INDEX)
			{
				return IShaderExpressionPtr(new GlobalShaderParmExpression(shaderParmNum));
			}
			else
			{
				throw new parser::ParseException("Shaderparm index out of bounds");
			}
		}
		else if (token == "time")
		{
			_tokeniser.nextToken(); // valid token, exhaust

			return IShaderExpressionPtr(new TimeExpression);
		}
		else if (token == "sound")
		{
			_tokeniser.nextToken(); // valid token, exhaust

			// No sound support so far
			return IShaderExpressionPtr(new ConstantExpression(0));
		}
		else if (boost::algorithm::iequals("fragmentprograms", token))
		{
			_tokeniser.nextToken(); // valid token, exhaust

			// There's no fragmentPrograms option in DR, let's assume true
			return IShaderExpressionPtr(new ConstantExpression(1));
		}
		else 
		{
			// Check if this keyword is a material lookup table
			TableDefinitionPtr table = GetShaderSystem()->getTableForName(token);

			if (table != NULL)
			{
				// Got it, this is a table name
				_tokeniser.nextToken(); // valid token, exhaust, this hasn't been done by the caller yet
				
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

					_tokeniser.nextToken(); // valid token, exhaust

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
	// Create an adapter which takes care of splitting the tokens into finer grains
	// The incoming DefTokeniser is not splitting up expressions like "3*4" without any whitespace in them
	expressions::ShaderExpressionTokeniser adapter(tokeniser);

	try
	{
		expressions::ShaderExpressionParser parser(adapter);
		return parser.getExpression();
	}
	catch (parser::ParseException& ex)
	{
		rWarning() << "[shaders] " << ex.what() << std::endl;
		return IShaderExpressionPtr();
	}
}

IShaderExpressionPtr ShaderExpression::createFromString(const std::string& exprStr)
{
	parser::BasicDefTokeniser<std::string> tokeniser(exprStr, parser::WHITESPACE, "{}(),");
	return createFromTokens(tokeniser);
}

} // namespace
