#pragma once

#include "igui.h"
#include <memory>
#include "string/convert.h"
#include <parser/DefTokeniser.h>

namespace gui
{

class GuiExpression;
typedef std::shared_ptr<GuiExpression> GuiExpressionPtr;

// Represents an expression found in the GUI code, which can be 
// a simple float, a string, a gui variable or a complex formula.
class GuiExpression
{
private:

public:
	GuiExpression();

	virtual float getFloatValue() = 0;
	virtual std::string getStringValue() = 0;

	static GuiExpressionPtr createFromString(const std::string& exprStr);

	static GuiExpressionPtr createFromTokens(parser::DefTokeniser& tokeniser);
};

template<typename ValueType>
class TypedExpression :
	public IGuiExpression<ValueType>
{
private:
	GuiExpressionPtr _contained;

public:
	TypedExpression(const GuiExpressionPtr& contained) :
		_contained(contained)
	{}

	virtual ValueType evaluate() override
	{
		return string::convert<ValueType>(_contained->getStringValue());
	}
};

// Boolean specialisation
template<>
class TypedExpression<bool> :
	public IGuiExpression<bool>
{
private:
	GuiExpressionPtr _contained;

public:
	TypedExpression(const GuiExpressionPtr& contained) :
		_contained(contained)
	{}

	virtual bool evaluate() override
	{
		return _contained->getFloatValue() != 0.0f;
	}
};

// An expression representing a constant floating point value
class FloatExpression :
	public GuiExpression,
	public ConstantExpression<float>
{
public:
	FloatExpression(float value) :
		ConstantExpression(value)
	{}

	virtual float getFloatValue()
	{
		return this->evaluate();
	}

	virtual std::string getStringValue()
	{
		return string::to_string(getFloatValue());
	}
};

// An expression representing a constant string value
class StringExpression :
	public GuiExpression,
	public ConstantExpression<std::string>
{
public:
	StringExpression(const std::string& value) :
		ConstantExpression(value)
	{}

	virtual float getFloatValue()
	{
		return string::convert<float>(getStringValue());
	}

	virtual std::string getStringValue()
	{
		return this->evaluate();
	}
};

// An expression representing a Vector4 value
// Translating between a quadruple of script expressions and IGuiExpression<Vector4>
class Vector4Expression :
	public IGuiExpression<Vector4>
{
private:
	std::vector<GuiExpressionPtr> _vec;

public:
	Vector4Expression(const GuiExpressionPtr& x, const GuiExpressionPtr& y, 
					  const GuiExpressionPtr& z, const GuiExpressionPtr& w) :
		_vec(4)
	{
		_vec[0] = x;
		_vec[1] = y;
		_vec[2] = z;
		_vec[3] = w;
	}

	virtual Vector4 evaluate() override
	{
		return Vector4(_vec[0]->getFloatValue(), _vec[1]->getFloatValue(), 
			_vec[2]->getFloatValue(), _vec[3]->getFloatValue());
	}
};

class GuiStateVariableExpression :
	public GuiExpression
{
private:
	std::string _variableName;
public:
	GuiStateVariableExpression(const std::string& variableName);

	virtual float getFloatValue() override;
	virtual std::string getStringValue() override;
};

}

