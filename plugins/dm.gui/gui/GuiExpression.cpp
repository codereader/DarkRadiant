#include "GuiExpression.h"

#include <list>
#include <stack>

#include "itextstream.h"

#include "string/trim.h"
#include "string/convert.h"
#include "string/predicate.h"

namespace gui
{

// Specialised implementations of the GuiExpression below

ConstantExpression::ConstantExpression(const std::string& stringValue) :
	GuiExpression(),
	_stringValue(stringValue),
	_floatValue(string::convert<float>(stringValue, 0.0f))
{}

ConstantExpression::ConstantExpression(float value) :
	GuiExpression(),
	_stringValue(string::to_string(value)),
	_floatValue(value)
{}

float ConstantExpression::getFloatValue()
{
	return _floatValue;
}

std::string ConstantExpression::getStringValue()
{
	return _stringValue;
}

// An expression referring to a GUI state variable
GuiStateVariableExpression::GuiStateVariableExpression(const std::string& variableName) :
	_variableName(variableName)
{}

float GuiStateVariableExpression::getFloatValue()
{
	return 0.0f; // TODO
}

std::string GuiStateVariableExpression::getStringValue()
{
	return ""; // TODO
}

namespace detail
{

// Abstract base class for an expression taking two sub-expression as arguments
// Additions, Multiplications, Divisions, Modulo, comparisons, etc.
class BinaryExpression :
	public GuiExpression
{
public:
	// The operator precedence, smaller values mean higher priority
	enum Precedence
	{
		LOGICAL_NOT = 0, // !
		MULTIPLICATION = 0,	// *
		DIVISION = 0,	// /
		MODULO = 0,	// %
		ADDITION = 1,	// +
		SUBTRACTION = 1,	// -
		RELATIONAL_COMPARISON = 2,	// > >= < <=
		EQUALITY_COMPARISON = 3,	// == !=
		LOGICAL_AND = 4,	// &&
		LOGICAL_OR = 5,	// ||
	};

protected:
	GuiExpressionPtr _a;
	GuiExpressionPtr _b;
	Precedence _precedence;

public:
	BinaryExpression(Precedence precedence,
		const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		GuiExpression(),
		_a(a),
		_b(b),
		_precedence(precedence)
	{}

	Precedence getPrecedence() const
	{
		return _precedence;
	}

	void setA(const GuiExpressionPtr& a)
	{
		_a = a;
	}

	void setB(const GuiExpressionPtr& b)
	{
		_b = b;
	}

	virtual std::string getStringValue() override
	{
		return string::to_string(getFloatValue());
	}
};
typedef std::shared_ptr<BinaryExpression> BinaryExpressionPtr;

// An expression adding the value of two expressions
class AddExpression :
	public BinaryExpression
{
public:
	AddExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(ADDITION, a, b)
	{}

	virtual float getFloatValue() override
	{
		return _a->getFloatValue() + _b->getFloatValue();
	}
};

// An expression subtracting the value of two expressions
class SubtractExpression :
	public BinaryExpression
{
public:
	SubtractExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(SUBTRACTION, a, b)
	{}

	virtual float getFloatValue() override
	{
		return _a->getFloatValue() - _b->getFloatValue();
	}
};

// An expression multiplying the value of two expressions
class MultiplyExpression :
	public BinaryExpression
{
public:
	MultiplyExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(MULTIPLICATION, a, b)
	{}

	virtual float getFloatValue() override
	{
		return _a->getFloatValue() * _b->getFloatValue();
	}
};

// An expression dividing the value of two expressions
class DivideExpression :
	public BinaryExpression
{
public:
	DivideExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(DIVISION, a, b)
	{}

	virtual float getFloatValue() override
	{
		return _a->getFloatValue() / _b->getFloatValue();
	}
};

// An expression returning modulo of A % B
class ModuloExpression :
	public BinaryExpression
{
public:
	ModuloExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(MODULO, a, b)
	{}

	virtual float getFloatValue() override
	{
		return fmod(_a->getFloatValue(), _b->getFloatValue());
	}
};

// An expression returning 1 if A < B, otherwise 0
class LesserThanExpression :
	public BinaryExpression
{
public:
	LesserThanExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(RELATIONAL_COMPARISON, a, b)
	{}

	virtual float getFloatValue() override
	{
		return _a->getFloatValue() < _b->getFloatValue() ? 1.0f : 0;
	}
};

// An expression returning 1 if A <= B, otherwise 0
class LesserThanOrEqualExpression :
	public BinaryExpression
{
public:
	LesserThanOrEqualExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(RELATIONAL_COMPARISON, a, b)
	{}

	virtual float getFloatValue() override
	{
		return _a->getFloatValue() <= _b->getFloatValue() ? 1.0f : 0;
	}
};

// An expression returning 1 if A > B, otherwise 0
class GreaterThanExpression :
	public BinaryExpression
{
public:
	GreaterThanExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(RELATIONAL_COMPARISON, a, b)
	{}

	virtual float getFloatValue() override
	{
		return _a->getFloatValue() > _b->getFloatValue() ? 1.0f : 0;
	}
};

// An expression returning 1 if A >= B, otherwise 0
class GreaterThanOrEqualExpression :
	public BinaryExpression
{
public:
	GreaterThanOrEqualExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(RELATIONAL_COMPARISON, a, b)
	{}

	virtual float getFloatValue() override
	{
		return _a->getFloatValue() >= _b->getFloatValue() ? 1.0f : 0;
	}
};

// An expression returning 1 if A == B, otherwise 0
class EqualityExpression :
	public BinaryExpression
{
public:
	EqualityExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(EQUALITY_COMPARISON, a, b)
	{}

	virtual float getFloatValue() override
	{
		return _a->getFloatValue() == _b->getFloatValue() ? 1.0f : 0;
	}
};

// An expression returning 1 if A != B, otherwise 0
class InequalityExpression :
	public BinaryExpression
{
public:
	InequalityExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(EQUALITY_COMPARISON, a, b)
	{}

	virtual float getFloatValue() override
	{
		return _a->getFloatValue() != _b->getFloatValue() ? 1.0f : 0;
	}
};

// An expression returning the opposite of the contained expression
class LogicalNotExpression :
	public BinaryExpression
{
private:
	GuiExpressionPtr _contained;

public:
	LogicalNotExpression(const GuiExpressionPtr& contained = GuiExpressionPtr()) :
		BinaryExpression(LOGICAL_NOT, contained)
	{}

	virtual float getFloatValue() override
	{
		return _a->getFloatValue() != 0.0f ? 1.0f : 0.0f;
	}

	virtual std::string getStringValue() override
	{
		return ""; // not implemented for string values
	}
};

// An expression returning 1 if both A and B are true (non-zero), otherwise 0
class LogicalAndExpression :
	public BinaryExpression
{
public:
	LogicalAndExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(LOGICAL_AND, a, b)
	{}

	virtual float getFloatValue() override
	{
		return (_a->getFloatValue() != 0 && _b->getFloatValue() != 0) ? 1.0f : 0;
	}
};

// An expression returning 1 if either A or B are true (non-zero), otherwise 0
class LogicalOrExpression :
	public BinaryExpression
{
public:
	LogicalOrExpression(const GuiExpressionPtr& a = GuiExpressionPtr(),
		const GuiExpressionPtr& b = GuiExpressionPtr()) :
		BinaryExpression(LOGICAL_OR, a, b)
	{}

	virtual float getFloatValue() override
	{
		return (_a->getFloatValue() != 0 || _b->getFloatValue() != 0) ? 1.0f : 0;
	}
};

// Fixme: Maybe merge this with ShaderExpressionTokeniser
class GuiExpressionTokeniser :
	public parser::DefTokeniser
{
private:
	parser::DefTokeniser& _tokeniser;

	// buffer containing tokens pulled from the wrapped tokeniser
	std::list<std::string> _buffer;

	const char* _delims;

public:
	GuiExpressionTokeniser(parser::DefTokeniser& tokeniser) :
		_tokeniser(tokeniser),
		_delims("+-%*/=!<>&|")
	{}

	bool hasMoreTokens() const override
	{
		return !_buffer.empty() || _tokeniser.hasMoreTokens();
	}

	std::string nextToken() override
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

	std::string peek() const override
	{
		if (!_buffer.empty())
		{
			// We have items in the buffer, return that one
			return _buffer.front();
		}

		// No items in the buffer, take a peek in the underlying stream
		std::string rawToken = _tokeniser.peek();

		if (rawToken.empty())
		{
			return rawToken; // don't tokenise empty strings, just return it as it is
		}

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

class GuiExpressionParser
{
private:
	GuiExpressionTokeniser& _tokeniser;

	typedef std::stack<GuiExpressionPtr> OperandStack;
	typedef std::stack<BinaryExpressionPtr> OperatorStack;

public:
	GuiExpressionParser(GuiExpressionTokeniser& tokeniser) :
		_tokeniser(tokeniser)
	{}

	GuiExpressionPtr getExpression()
	{
		// The local variable and operator stack
		OperandStack operands;
		OperatorStack operators;

		enum {
			SearchingForOperand,
			SearchingForOperator,
		} searchState;

		searchState = SearchingForOperand;

		while (_tokeniser.hasMoreTokens())
		{
			if (searchState == SearchingForOperand)
			{
				// Don't actually pull the token from the tokeniser, we might want to 
				// return the tokeniser to the caller if the token is not part of the expression
				// The token will be exhausted from the stream once it is recognised as keyword
				std::string token = _tokeniser.peek();

				// The parsed operand
				GuiExpressionPtr term;

				if (token == "(")
				{
					_tokeniser.nextToken(); // valid token, exhaust

					// New scope, treat this as new expression
					term = getExpression();
				}
				else if (token == ")")
				{
					_tokeniser.nextToken(); // valid token, exhaust

					// End of scope reached, break the loop and roll up the expression
					break;
				}
				else if (string::starts_with(token, "gui::"))
				{
					// This is a GUI state variable
					_tokeniser.nextToken();
					term = std::make_shared<GuiStateVariableExpression>(token.substr(5));
				}
				// If this is a + or -, take it as a sign operator
				else if (token == "+")
				{
					// A leading +, just ignore it
					_tokeniser.nextToken();
					continue;
				}
				else if (token == "-")
				{
					// A leading -, interpret it as -1 *
					operands.push(std::make_shared<ConstantExpression>(-1.0f));
					operators.push(std::make_shared<MultiplyExpression>());

					// Discard the - token
					_tokeniser.nextToken();
					continue;
				}
				else
				{
					// This might either be a float or a string constant
					term = std::make_shared<ConstantExpression>(_tokeniser.nextToken());
				}
				
				if (!term)
				{
					break; // We've run out of terms
				}

				// The token has already been pulled from the tokeniser
				operands.push(term);

				// Look for an operator next
				searchState = SearchingForOperator;
				continue;
			}
			else if (searchState == SearchingForOperator)
			{
				// Get an operator
				BinaryExpressionPtr op = getOperator();

				if (op)
				{
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
					searchState = SearchingForOperand; // back to operand search mode
					continue;
				}

				// Not an operator, break the loop
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

		GuiExpressionPtr rv = operands.top();
		operands.pop();

		assert(operands.empty()); // there should be nothing left on the stack

		return rv;
	}

private:
	void finaliseOperator(OperandStack& operands, OperatorStack& operators)
	{
		const BinaryExpressionPtr& op = operators.top();

		// Set operand B first, we're dealing with a LIFO stack
		// Do not push a second operand in the case of a logical NOT
		if (!std::dynamic_pointer_cast<LogicalNotExpression>(op))
		{
			// Need two operands for an operator
			if (operands.size() < 2)
			{
				throw parser::ParseException("Too few operands for operator.");
			}

			op->setB(operands.top());
			operands.pop();
		}
		else
		{
			// Need at least one operand for a NOT operator
			if (operands.size() < 1)
			{
				throw parser::ParseException("Too few operands for ! operator.");
			}
		}

		op->setA(operands.top());
		operands.pop();

		// Push the result back on the stack, as operand
		operands.push(op);

		// Remove the operator, after we've copied it to the operand stack
		operators.pop();
	}

	// Try to get a valid expression from the token. If the token was found to be valid
	// The token is actually pulled from the tokeniser using nextToken()
	GuiExpressionPtr getTerm(const std::string& token)
	{
		std::string tokenCleaned = string::trim_copy(token, "\"");

		_tokeniser.nextToken(); // valid token, exhaust

		return std::make_shared<ConstantExpression>(tokenCleaned);
	}

	// Helper routines
	BinaryExpressionPtr getOperator()
	{
		if (!_tokeniser.hasMoreTokens())
		{
			return BinaryExpressionPtr();
		}

		std::string token = _tokeniser.peek();

		if (token == "+")
		{
			_tokeniser.nextToken();
			return std::make_shared<AddExpression>();
		}
		else if (token == "-")
		{
			_tokeniser.nextToken();
			return std::make_shared<SubtractExpression>();
		}
		else if (token == "*")
		{
			_tokeniser.nextToken();
			return std::make_shared<MultiplyExpression>();
		}
		else if (token == "/")
		{
			_tokeniser.nextToken();
			return std::make_shared<DivideExpression>();
		}
		else if (token == "%")
		{
			_tokeniser.nextToken();
			return std::make_shared<ModuloExpression>();
		}
		else if (token == "<")
		{
			_tokeniser.nextToken();

			if (_tokeniser.peek() == "=")
			{
				_tokeniser.nextToken();
				return std::make_shared<LesserThanOrEqualExpression>();
			}

			return std::make_shared<LesserThanExpression>();
		}
		else if (token == ">")
		{
			_tokeniser.nextToken();

			if (_tokeniser.peek() == "=")
			{
				_tokeniser.nextToken();
				return std::make_shared<GreaterThanOrEqualExpression>();
			}

			return std::make_shared<GreaterThanExpression>();
		}
		else if (token == "=")
		{
			_tokeniser.nextToken();

			if (_tokeniser.peek() == "=")
			{
				_tokeniser.nextToken();
				return std::make_shared<EqualityExpression>();
			}

			rError() << "Assignment operator '=' found in expression, did you mean '=='?" << std::endl;
		}
		else if (token == "!")
		{
			_tokeniser.nextToken();

			if (_tokeniser.peek() == "=")
			{
				_tokeniser.nextToken();
				return std::make_shared<InequalityExpression>();
			}

			return std::make_shared<LogicalNotExpression>();
		}
		else if (token == "&")
		{
			_tokeniser.nextToken();

			if (_tokeniser.peek() == "&")
			{
				_tokeniser.nextToken();
				return std::make_shared<LogicalAndExpression>();
			}

			rError() << "Bit-wise operator '&' found in expression, did you mean '&&'?" << std::endl;
		}
		else if (token == "|")
		{
			_tokeniser.nextToken();

			if (_tokeniser.peek() == "|")
			{
				_tokeniser.nextToken();
				return std::make_shared<LogicalOrExpression>();
			}

			rError() << "Bit-wise operator '|' found in expression, did you mean '||'?" << std::endl;
		}

		return BinaryExpressionPtr();
	}
};

}

GuiExpression::GuiExpression()
{}

GuiExpressionPtr GuiExpression::createFromString(const std::string& exprStr)
{
	parser::BasicDefTokeniser<std::string> tokeniser(exprStr, parser::WHITESPACE, "{}(),");
	return createFromTokens(tokeniser);
}

GuiExpressionPtr GuiExpression::createFromTokens(parser::DefTokeniser& tokeniser)
{
	// Create an adapter which takes care of splitting the tokens into finer grains
	// The incoming DefTokeniser is not splitting up expressions like "3*4" without any whitespace in them
	detail::GuiExpressionTokeniser adapter(tokeniser);

	try
	{
		detail::GuiExpressionParser parser(adapter);
		return parser.getExpression();
	}
	catch (parser::ParseException& ex)
	{
		rWarning() << "[GuiExpressionParser] " << ex.what() << std::endl;
		return GuiExpressionPtr();
	}
}

}

