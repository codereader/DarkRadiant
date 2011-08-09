#pragma once

#include "ishaders.h"
#include "parser/Tokeniser.h"
#include "TableDefinition.h"

namespace shaders
{

// The base class containing the factory methods
class ShaderExpression :
	public IShaderExpression
{
public:
	static IShaderExpressionPtr createFromString(const std::string& exprStr);
};

// Detail namespace
namespace expressions
{

// Specialised implementations of the ShaderExpression below

class ShaderParmExpression :
	public ShaderExpression
{
private:
	// The attached entity's shaderparm number to retrieve
	int _parmNum;
public:
	ShaderParmExpression(int parmNum) :
		ShaderExpression(),
		_parmNum(parmNum)
	{}

	virtual float getValue()
	{
		// not implemented yet
		return 0.0f;
	}

	virtual float evaluate()
	{
		// do nothing so far
		return getValue();
	}
};

// An expression returning the current (game) time as result
class TimeExpression :
	public ShaderExpression
{
public:
	TimeExpression() :
		ShaderExpression()
	{}

	virtual float getValue()
	{
		// not implemented yet
		return 0.0f;
	}

	virtual float evaluate()
	{
		// do nothing so far
		return getValue();
	}
};

// An expression representing a constant floating point number
class ConstantExpression :
	public ShaderExpression
{
private:
	float _value;

public:
	ConstantExpression(float value) :
		ShaderExpression(),
		_value(value)
	{}

	virtual float getValue()
	{
		return _value;
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression looking up a value in a table def
class TableLookupExpression :
	public ShaderExpression
{
private:
	TableDefinitionPtr _tableDef;
	IShaderExpressionPtr _lookupExpr;

public:
	// Pass the table and the expression used to perform the lookup 
	TableLookupExpression(const TableDefinitionPtr& tableDef, 
						  const IShaderExpressionPtr& lookupExpr) :
		ShaderExpression(),
		_tableDef(tableDef),
		_lookupExpr(lookupExpr)
	{
		assert(_tableDef);
		assert(_lookupExpr);
	}

	virtual float getValue()
	{
		float lookupVal = _lookupExpr->getValue();
		return _tableDef->getValue(lookupVal);
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// Abstract base class for an expression taking two sub-expression as arguments
// Additions, Multiplications, Divisions, Modulo, comparisons, etc.
class BinaryExpression :
	public ShaderExpression
{
public:
	// The operator precedence, smaller values mean higher priority
	enum Precedence
	{
		MULTIPLICATION			= 0,	// *
		DIVISION				= 0,	// /
		MODULO					= 0,	// %
		ADDITION				= 1,	// +
		SUBTRACTION				= 1,	// -
		RELATIONAL_COMPARISON	= 2,	// > >= < <=
		EQUALITY_COMPARISON		= 3,	// == !=
		LOGICAL_AND				= 4,	// &&
		LOGICAL_OR				= 5,	// ||
	};

protected:
	IShaderExpressionPtr _a;
	IShaderExpressionPtr _b;
	Precedence _precedence;

public:
	BinaryExpression(Precedence precedence,
					 const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
				     const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		ShaderExpression(),
		_a(a),
		_b(b),
		_precedence(precedence)
	{}

	Precedence getPrecedence() const
	{
		return _precedence;
	}

	void setA(const IShaderExpressionPtr& a)
	{
		_a = a;
	}

	void setB(const IShaderExpressionPtr& b)
	{
		_b = b;
	}
};
typedef boost::shared_ptr<BinaryExpression> BinaryExpressionPtr;

// An expression adding the value of two expressions
class AddExpression :
	public BinaryExpression
{
public:
	AddExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
				  const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(ADDITION, a, b)
	{}

	virtual float getValue()
	{
		return _a->getValue() + _b->getValue();
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression subtracting the value of two expressions
class SubtractExpression :
	public BinaryExpression
{
public:
	SubtractExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
					   const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(SUBTRACTION, a, b)
	{}

	virtual float getValue()
	{
		return _a->getValue() - _b->getValue();
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression multiplying the value of two expressions
class MultiplyExpression :
	public BinaryExpression
{
public:
	MultiplyExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
					   const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(MULTIPLICATION, a, b)
	{}

	virtual float getValue()
	{
		return _a->getValue() * _b->getValue();
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression dividing the value of two expressions
class DivideExpression :
	public BinaryExpression
{
public:
	DivideExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
					 const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(DIVISION, a, b)
	{}

	virtual float getValue()
	{
		return _a->getValue() / _b->getValue();
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression returning modulo of A % B
class ModuloExpression :
	public BinaryExpression
{
public:
	ModuloExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
					 const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(MODULO, a, b)
	{}

	virtual float getValue()
	{
		return fmod(_a->getValue(), _b->getValue());
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression returning 1 if A < B, otherwise 0
class LesserThanExpression :
	public BinaryExpression
{
public:
	LesserThanExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
						 const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(RELATIONAL_COMPARISON, a, b)
	{}

	virtual float getValue()
	{
		return _a->getValue() < _b->getValue() ? 1.0f : 0;
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression returning 1 if A <= B, otherwise 0
class LesserThanOrEqualExpression :
	public BinaryExpression
{
public:
	LesserThanOrEqualExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
								const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(RELATIONAL_COMPARISON, a, b)
	{}

	virtual float getValue()
	{
		return _a->getValue() <= _b->getValue() ? 1.0f : 0;
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression returning 1 if A > B, otherwise 0
class GreaterThanExpression :
	public BinaryExpression
{
public:
	GreaterThanExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
						  const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(RELATIONAL_COMPARISON, a, b)
	{}

	virtual float getValue()
	{
		return _a->getValue() > _b->getValue() ? 1.0f : 0;
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression returning 1 if A >= B, otherwise 0
class GreaterThanOrEqualExpression :
	public BinaryExpression
{
public:
	GreaterThanOrEqualExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
								 const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(RELATIONAL_COMPARISON, a, b)
	{}

	virtual float getValue()
	{
		return _a->getValue() >= _b->getValue() ? 1.0f : 0;
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression returning 1 if A == B, otherwise 0
class EqualityExpression :
	public BinaryExpression
{
public:
	EqualityExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
					   const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(EQUALITY_COMPARISON, a, b)
	{}

	virtual float getValue()
	{
		return _a->getValue() == _b->getValue() ? 1.0f : 0;
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression returning 1 if A != B, otherwise 0
class InequalityExpression :
	public BinaryExpression
{
public:
	InequalityExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
					     const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(EQUALITY_COMPARISON, a, b)
	{}

	virtual float getValue()
	{
		return _a->getValue() != _b->getValue() ? 1.0f : 0;
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression returning 1 if both A and B are true (non-zero), otherwise 0
class LogicalAndExpression :
	public BinaryExpression
{
public:
	LogicalAndExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
					     const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(LOGICAL_AND, a, b)
	{}

	virtual float getValue()
	{
		return (_a->getValue() != 0 && _b->getValue() != 0) ? 1.0f : 0;
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

// An expression returning 1 if either A or B are true (non-zero), otherwise 0
class LogicalOrExpression :
	public BinaryExpression
{
public:
	LogicalOrExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
					    const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(LOGICAL_OR, a, b)
	{}

	virtual float getValue()
	{
		return (_a->getValue() != 0 || _b->getValue() != 0) ? 1.0f : 0;
	}

	virtual float evaluate()
	{
		// no saving so far
		return getValue();
	}
};

} // namespace

} // namespace
