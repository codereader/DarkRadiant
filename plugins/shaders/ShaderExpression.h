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
		ADD,
		MUL,
	};

protected:
	IShaderExpressionPtr _a;
	IShaderExpressionPtr _b;
	Precedence _precedence;

public:
	// Pass the table and the expression used to perform the lookup 
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
	// Pass the table and the expression used to perform the lookup 
	AddExpression(const IShaderExpressionPtr& a = IShaderExpressionPtr(), 
				  const IShaderExpressionPtr& b = IShaderExpressionPtr()) :
		BinaryExpression(ADD, a, b)
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

} // namespace

} // namespace
