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

protected:
    ShaderExpression(const ShaderExpression& other) :
        ShaderExpression() // use default ctor to initialise the members
    {
        // Inherit the parentheses flag
        _surroundedByParentheses = other._surroundedByParentheses;
    }

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
    static IShaderExpression::Ptr createTableLookup(const ITableDefinition::Ptr& table, const IShaderExpression::Ptr& lookup);

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

    ShaderParmExpression(const ShaderParmExpression& other) :
        ShaderExpression(other),
        _parmNum(other._parmNum)
    {}

	float getValue(std::size_t time) override
	{
        // RGBA _color parms [0-3] have default value 1.0
        if (_parmNum < 4) return 1.0f;

		// The rest of the parms is 0 by default
		return 0.0f;
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return entity.getShaderParm(_parmNum);
	}

    virtual std::string convertToString() override
    {
        return fmt::format("parm{0}", _parmNum);
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<ShaderParmExpression>(*this);
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

    GlobalShaderParmExpression(const GlobalShaderParmExpression& other) :
        ShaderExpression(other),
        _parmNum(other._parmNum)
    {}

	float getValue(std::size_t time) override
	{
		// globalNN is 0 without entity
		return 0.0f;
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return getValue(time);
	}

    virtual std::string convertToString() override
    {
        return fmt::format("global{0}", _parmNum);
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<GlobalShaderParmExpression>(*this);
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

    TimeExpression(const TimeExpression& other) :
        ShaderExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return time / 1000.0f; // convert msecs to secs
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return getValue(time);
	}

    virtual std::string convertToString() override
    {
        return "time";
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<TimeExpression>(*this);
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

    ConstantExpression(const ConstantExpression& other) :
        ShaderExpression(other),
        _value(other._value)
    {}

	float getValue(std::size_t time) override
	{
		return _value;
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return getValue(time);
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0}", _value);
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<ConstantExpression>(*this);
    }
};

// An expression looking up a value in a table def
class TableLookupExpression :
	public ShaderExpression
{
private:
    ITableDefinition::Ptr _tableDef;
	IShaderExpression::Ptr _lookupExpr;

public:
	// Pass the table and the expression used to perform the lookup
	TableLookupExpression(const ITableDefinition::Ptr& tableDef,
						  const IShaderExpression::Ptr& lookupExpr) :
		ShaderExpression(),
		_tableDef(tableDef),
		_lookupExpr(lookupExpr)
	{
		assert(_tableDef);
		assert(_lookupExpr);
	}

    TableLookupExpression(const TableLookupExpression& other) :
        ShaderExpression(other),
        _tableDef(other._tableDef),
        _lookupExpr(other._lookupExpr ? other._lookupExpr->clone() : Ptr())
    {}

	float getValue(std::size_t time) override
	{
		float lookupVal = _lookupExpr->getValue(time);
		return _tableDef->getValue(lookupVal);
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		float lookupVal = _lookupExpr->getValue(time, entity);
		return _tableDef->getValue(lookupVal);
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0}[{1}]", _tableDef->getDeclName(), _lookupExpr->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<TableLookupExpression>(*this);
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

protected:
	BinaryExpression(Precedence precedence,
					 const IShaderExpression::Ptr& a = IShaderExpression::Ptr(),
				     const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
		ShaderExpression(),
		_a(a),
		_b(b),
		_precedence(precedence)
	{}

    BinaryExpression(const BinaryExpression& other) :
        ShaderExpression(other),
        _a(other._a),
        _b(other._b),
        _precedence(other._precedence)
    {}

public:

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

    AddExpression(const AddExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return _a->getValue(time) + _b->getValue(time);
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return _a->getValue(time, entity) + _b->getValue(time, entity);
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} + {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<AddExpression>(*this);
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

    SubtractExpression(const SubtractExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return _a->getValue(time) - _b->getValue(time);
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return _a->getValue(time, entity) - _b->getValue(time, entity);
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} - {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<SubtractExpression>(*this);
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

    MultiplyExpression(const MultiplyExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return _a->getValue(time) * _b->getValue(time);
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return _a->getValue(time, entity) * _b->getValue(time, entity);
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} * {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<MultiplyExpression>(*this);
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

    DivideExpression(const DivideExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return _a->getValue(time) / _b->getValue(time);
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return _a->getValue(time, entity) / _b->getValue(time, entity);
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} / {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<DivideExpression>(*this);
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

    ModuloExpression(const ModuloExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return fmod(_a->getValue(time), _b->getValue(time));
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return fmod(_a->getValue(time, entity), _b->getValue(time, entity));
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} % {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<ModuloExpression>(*this);
    }
};

// An expression returning 1 if A < B, otherwise 0
class LessThanExpression :
	public BinaryExpression
{
public:
	LessThanExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(),
                       const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
		BinaryExpression(RELATIONAL_COMPARISON, a, b)
	{}

    LessThanExpression(const LessThanExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return _a->getValue(time) < _b->getValue(time) ? 1.0f : 0;
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return _a->getValue(time, entity) < _b->getValue(time, entity) ? 1.0f : 0;
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} < {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<LessThanExpression>(*this);
    }
};

// An expression returning 1 if A <= B, otherwise 0
class LessThanOrEqualExpression :
	public BinaryExpression
{
public:
	LessThanOrEqualExpression(const IShaderExpression::Ptr& a = IShaderExpression::Ptr(),
                              const IShaderExpression::Ptr& b = IShaderExpression::Ptr()) :
		BinaryExpression(RELATIONAL_COMPARISON, a, b)
	{}

    LessThanOrEqualExpression(const LessThanOrEqualExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return _a->getValue(time) <= _b->getValue(time) ? 1.0f : 0;
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return _a->getValue(time, entity) <= _b->getValue(time, entity) ? 1.0f : 0;
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} <= {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<LessThanOrEqualExpression>(*this);
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

    GreaterThanExpression(const GreaterThanExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return _a->getValue(time) > _b->getValue(time) ? 1.0f : 0;
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return _a->getValue(time, entity) > _b->getValue(time, entity) ? 1.0f : 0;
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} > {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<GreaterThanExpression>(*this);
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

    GreaterThanOrEqualExpression(const GreaterThanOrEqualExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return _a->getValue(time) >= _b->getValue(time) ? 1.0f : 0;
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return _a->getValue(time, entity) >= _b->getValue(time, entity) ? 1.0f : 0;
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} >= {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<GreaterThanOrEqualExpression>(*this);
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

    EqualityExpression(const EqualityExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return _a->getValue(time) == _b->getValue(time) ? 1.0f : 0;
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return _a->getValue(time, entity) == _b->getValue(time, entity) ? 1.0f : 0;
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} == {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<EqualityExpression>(*this);
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

    InequalityExpression(const InequalityExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return _a->getValue(time) != _b->getValue(time) ? 1.0f : 0;
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return _a->getValue(time, entity) != _b->getValue(time, entity) ? 1.0f : 0;
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} != {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<InequalityExpression>(*this);
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

    LogicalAndExpression(const LogicalAndExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return (_a->getValue(time) != 0 && _b->getValue(time) != 0) ? 1.0f : 0;
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return (_a->getValue(time, entity) != 0 && _b->getValue(time, entity) != 0) ? 1.0f : 0;
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} && {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<LogicalAndExpression>(*this);
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

    LogicalOrExpression(const LogicalOrExpression& other) :
        BinaryExpression(other)
    {}

	float getValue(std::size_t time) override
	{
		return (_a->getValue(time) != 0 || _b->getValue(time) != 0) ? 1.0f : 0;
	}

	float getValue(std::size_t time, const IRenderEntity& entity) override
	{
		return (_a->getValue(time, entity) != 0 || _b->getValue(time, entity) != 0) ? 1.0f : 0;
	}

    virtual std::string convertToString() override
    {
        return fmt::format("{0} || {1}", _a->getExpressionString(), _b->getExpressionString());
    }

    virtual Ptr clone() const override
    {
        return std::make_shared<LogicalOrExpression>(*this);
    }
};

} // namespace

} // namespace
