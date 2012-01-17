#pragma once

#include "ishaders.h"
#include "irender.h"
#include "parser/DefTokeniser.h"
#include "TableDefinition.h"

namespace shaders
{

// The base class containing the factory methods
class ShaderExpression :
	public IShaderExpression
{
	// The register index we're writing to (-1 by default)
	int _index;

	// The register we're writing to
	Registers* _registers;

public:
	ShaderExpression() :
		_index(-1),
		_registers(NULL)
	{}

	// Base implementations
	virtual float evaluate(std::size_t time)
	{
		// Evaluate this register and write it into the respective register index
		float val = getValue(time);

		if (_registers != NULL)
		{
			(*_registers)[_index] = val;
		}

		return val;
	}

	virtual float evaluate(std::size_t time, const IRenderEntity& entity)
	{
		// Evaluate this register and write it into the respective register index
		float val = getValue(time, entity);

		if (_registers != NULL)
		{
			(*_registers)[_index] = val;
		}

		return val;
	}

	std::size_t linkToRegister(Registers& registers) 
	{
		_registers = &registers;

		// Allocate a new register
		registers.push_back(0);

		// Save the index to the newly allocated register
		_index = static_cast<int>(registers.size() - 1);

		return _index;
	}

	static IShaderExpressionPtr createFromString(const std::string& exprStr);

	static IShaderExpressionPtr createFromTokens(parser::DefTokeniser& tokeniser);
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

	virtual float getValue(std::size_t time)
	{
		// parmNN is 0 without entity
		return 0.0f;
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return entity.getShaderParm(_parmNum);
	}
};

class GlobalShaderParmExpression :
	public ShaderExpression
{
private:
	// The global shaderparm number to retrieve
	int _parmNum;
public:
	GlobalShaderParmExpression(int parmNum) :
		ShaderExpression(),
		_parmNum(parmNum)
	{}

	virtual float getValue(std::size_t time)
	{
		// globalNN is 0 without entity
		return 0.0f;
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return getValue(time);
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

	virtual float getValue(std::size_t time)
	{
		return time / 1000.0f; // convert msecs to secs
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return getValue(time);
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

	virtual float getValue(std::size_t time)
	{
		return _value;
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return getValue(time);
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

	virtual float getValue(std::size_t time)
	{
		float lookupVal = _lookupExpr->getValue(time);
		return _tableDef->getValue(lookupVal);
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		float lookupVal = _lookupExpr->getValue(time, entity);
		return _tableDef->getValue(lookupVal);
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

	virtual float getValue(std::size_t time)
	{
		return _a->getValue(time) + _b->getValue(time);
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return _a->getValue(time, entity) + _b->getValue(time, entity);
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

	virtual float getValue(std::size_t time)
	{
		return _a->getValue(time) - _b->getValue(time);
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return _a->getValue(time, entity) - _b->getValue(time, entity);
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

	virtual float getValue(std::size_t time)
	{
		return _a->getValue(time) * _b->getValue(time);
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return _a->getValue(time, entity) * _b->getValue(time, entity);
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

	virtual float getValue(std::size_t time)
	{
		return _a->getValue(time) / _b->getValue(time);
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return _a->getValue(time, entity) / _b->getValue(time, entity);
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

	virtual float getValue(std::size_t time)
	{
		return fmod(_a->getValue(time), _b->getValue(time));
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return fmod(_a->getValue(time, entity), _b->getValue(time, entity));
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

	virtual float getValue(std::size_t time)
	{
		return _a->getValue(time) < _b->getValue(time) ? 1.0f : 0;
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return _a->getValue(time, entity) < _b->getValue(time, entity) ? 1.0f : 0;
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

	virtual float getValue(std::size_t time)
	{
		return _a->getValue(time) <= _b->getValue(time) ? 1.0f : 0;
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return _a->getValue(time, entity) <= _b->getValue(time, entity) ? 1.0f : 0;
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

	virtual float getValue(std::size_t time)
	{
		return _a->getValue(time) > _b->getValue(time) ? 1.0f : 0;
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return _a->getValue(time, entity) > _b->getValue(time, entity) ? 1.0f : 0;
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

	virtual float getValue(std::size_t time)
	{
		return _a->getValue(time) >= _b->getValue(time) ? 1.0f : 0;
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return _a->getValue(time, entity) >= _b->getValue(time, entity) ? 1.0f : 0;
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

	virtual float getValue(std::size_t time)
	{
		return _a->getValue(time) == _b->getValue(time) ? 1.0f : 0;
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return _a->getValue(time, entity) == _b->getValue(time, entity) ? 1.0f : 0;
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

	virtual float getValue(std::size_t time)
	{
		return _a->getValue(time) != _b->getValue(time) ? 1.0f : 0;
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return _a->getValue(time, entity) != _b->getValue(time, entity) ? 1.0f : 0;
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

	virtual float getValue(std::size_t time)
	{
		return (_a->getValue(time) != 0 && _b->getValue(time) != 0) ? 1.0f : 0;
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return (_a->getValue(time, entity) != 0 && _b->getValue(time, entity) != 0) ? 1.0f : 0;
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

	virtual float getValue(std::size_t time)
	{
		return (_a->getValue(time) != 0 || _b->getValue(time) != 0) ? 1.0f : 0;
	}

	virtual float getValue(std::size_t time, const IRenderEntity& entity)
	{
		return (_a->getValue(time, entity) != 0 || _b->getValue(time, entity) != 0) ? 1.0f : 0;
	}
};

} // namespace

} // namespace
