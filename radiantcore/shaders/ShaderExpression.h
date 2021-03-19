#pragma once

#include "ishaders.h"
#include "irender.h"
#include "parser/DefTokeniser.h"
#include "fmt/format.h"
#include "string/convert.h"
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

    bool _surroundedByParentheses;

public:
	ShaderExpression() :
		_index(-1),
		_registers(nullptr),
        _surroundedByParentheses(false)
	{}

	// Base implementations
	virtual float evaluate(std::size_t time) override
	{
		// Evaluate this register and write it into the respective register index
		float val = getValue(time);

		if (_registers != nullptr)
		{
			(*_registers)[_index] = val;
		}

		return val;
	}

	virtual float evaluate(std::size_t time, const IRenderEntity& entity) override
	{
		// Evaluate this register and write it into the respective register index
		float val = getValue(time, entity);

		if (_registers != nullptr)
		{
			(*_registers)[_index] = val;
		}

		return val;
	}

	std::size_t linkToRegister(Registers& registers) override
	{
		_registers = &registers;

		// Allocate a new register
		registers.push_back(0);

		// Save the index to the newly allocated register
		_index = static_cast<int>(registers.size() - 1);

		return _index;
	}

    void linkToSpecificRegister(Registers& registers, std::size_t index) override
    {
        _registers = &registers;
        _index = static_cast<int>(index);
    }

    bool isLinked() const override
    {
        return _index != -1;
    }

    std::size_t unlinkFromRegisters() override
    {
        _registers = nullptr;
        
        auto oldIndex = _index;
        _index = -1;

        return static_cast<std::size_t>(oldIndex);
    }

	static IShaderExpression::Ptr createFromString(const std::string& exprStr);

	static IShaderExpression::Ptr createFromTokens(parser::DefTokeniser& tokeniser);

    static IShaderExpression::Ptr createConstant(float constantValue);
    static IShaderExpression::Ptr createAddition(const IShaderExpression::Ptr& a, const IShaderExpression::Ptr& b);
    static IShaderExpression::Ptr createMultiplication(const IShaderExpression::Ptr& a, const IShaderExpression::Ptr& b);
    static IShaderExpression::Ptr createTableLookup(const TableDefinitionPtr& table, const IShaderExpression::Ptr& lookup);

    virtual std::string getExpressionString() override
    {
        return _surroundedByParentheses ? fmt::format("({0})", convertToString()) : convertToString();
    }

    void setIsSurroundedByParentheses(bool isSurrounded)
    {
        _surroundedByParentheses = isSurrounded;
    }

    // To be implemented by the subclasses
    virtual std::string convertToString() = 0;
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

    virtual std::string convertToString() override
    {
        return fmt::format("parm{0}", _parmNum);
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

    virtual std::string convertToString() override
    {
        return fmt::format("global{0}", _parmNum);
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

    virtual std::string convertToString() override
    {
        return "time";
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0}", _value);
    }
};

// An expression looking up a value in a table def
class TableLookupExpression :
	public ShaderExpression
{
private:
	TableDefinitionPtr _tableDef;
	IShaderExpression::Ptr _lookupExpr;

public:
	// Pass the table and the expression used to perform the lookup 
	TableLookupExpression(const TableDefinitionPtr& tableDef, 
						  const IShaderExpression::Ptr& lookupExpr) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0}[{1}]", _tableDef->getName(), _lookupExpr->getExpressionString());
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
	IShaderExpression::Ptr _a;
	IShaderExpression::Ptr _b;
	Precedence _precedence;

public:
	BinaryExpression(Precedence precedence,
					 const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
				     const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
		ShaderExpression(),
		_a(a),
		_b(b),
		_precedence(precedence)
	{}

	Precedence getPrecedence() const
	{
		return _precedence;
	}

	void setA(const IShaderExpression::Ptr& a)
	{
		_a = a;
	}

	void setB(const IShaderExpression::Ptr& b)
	{
		_b = b;
	}
};
typedef std::shared_ptr<BinaryExpression> BinaryExpressionPtr;

// An expression adding the value of two expressions
class AddExpression :
	public BinaryExpression
{
public:
	AddExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
				  const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} + {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

// An expression subtracting the value of two expressions
class SubtractExpression :
	public BinaryExpression
{
public:
	SubtractExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
					   const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} - {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

// An expression multiplying the value of two expressions
class MultiplyExpression :
	public BinaryExpression
{
public:
	MultiplyExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
					   const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} * {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

// An expression dividing the value of two expressions
class DivideExpression :
	public BinaryExpression
{
public:
	DivideExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
					 const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} / {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

// An expression returning modulo of A % B
class ModuloExpression :
	public BinaryExpression
{
public:
	ModuloExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
					 const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} % {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

// An expression returning 1 if A < B, otherwise 0
class LesserThanExpression :
	public BinaryExpression
{
public:
	LesserThanExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
						 const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} < {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

// An expression returning 1 if A <= B, otherwise 0
class LesserThanOrEqualExpression :
	public BinaryExpression
{
public:
	LesserThanOrEqualExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
								const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} <= {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

// An expression returning 1 if A > B, otherwise 0
class GreaterThanExpression :
	public BinaryExpression
{
public:
	GreaterThanExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
						  const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} > {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

// An expression returning 1 if A >= B, otherwise 0
class GreaterThanOrEqualExpression :
	public BinaryExpression
{
public:
	GreaterThanOrEqualExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
								 const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} >= {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

// An expression returning 1 if A == B, otherwise 0
class EqualityExpression :
	public BinaryExpression
{
public:
	EqualityExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
					   const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} == {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

// An expression returning 1 if A != B, otherwise 0
class InequalityExpression :
	public BinaryExpression
{
public:
	InequalityExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
					     const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} != {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

// An expression returning 1 if both A and B are true (non-zero), otherwise 0
class LogicalAndExpression :
	public BinaryExpression
{
public:
	LogicalAndExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
					     const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} && {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

// An expression returning 1 if either A or B are true (non-zero), otherwise 0
class LogicalOrExpression :
	public BinaryExpression
{
public:
	LogicalOrExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(), 
					    const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
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

    virtual std::string convertToString() override
    {
        return fmt::format("{0} || {1}", _a->getExpressionString(), _b->getExpressionString());
    }
};

} // namespace

} // namespace
