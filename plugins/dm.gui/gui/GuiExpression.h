#pragma once

#include "igui.h"
#include <memory>
#include "string/convert.h"
#include <parser/DefTokeniser.h>

namespace gui
{

class GuiExpression;
typedef std::shared_ptr<GuiExpression> GuiExpressionPtr;

// Represents a right-hand expression found in the GUI code, which can be 
// a simple float, a string, a gui variable or a complex formula.
class GuiExpression
{
protected:
	sigc::signal<void> _sigValueChanged;

public:
	GuiExpression();

	virtual float getFloatValue() = 0;
	virtual std::string getStringValue() = 0;

	// Sub-expressions or clients can subscribe to get notified about changes
	sigc::signal<void>& signal_valueChanged()
	{
		return _sigValueChanged;
	}

	static GuiExpressionPtr CreateFromString(IGui& gui, const std::string& exprStr);

	static GuiExpressionPtr CreateFromTokens(IGui& gui, parser::DefTokeniser& tokeniser);
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

	virtual float getFloatValue() override
	{
		return this->evaluate();
	}

	virtual std::string getStringValue() override
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

	virtual float getFloatValue() override
	{
		return string::convert<float>(getStringValue());
	}

	virtual std::string getStringValue() override
	{
		return this->evaluate();
	}
};

// Adapter class converting a generic GuiExpression to a IGuiExpression of a given type
// making use of the string conversion templates in the string/conv.h header.
template<typename ValueType>
class TypedExpression :
	public IGuiExpression<ValueType>
{
private:
	GuiExpressionPtr _contained;

	sigc::signal<void> _sigValueChanged;

public:
	TypedExpression(const GuiExpressionPtr& contained) :
		_contained(contained)
	{
		if (_contained)
		{
			// Connect the changed signal of the contained expression
			// to fire our own signal
			_contained->signal_valueChanged().connect([this]()
			{
				signal_valueChanged().emit();
			});
		}
	}

	// The generic evaluate() implementation will try to cast the string
	// value of the GuiExpression to the desired ValueType
	virtual ValueType evaluate() override
	{
		return string::convert<ValueType>(_contained->getStringValue());
	}

	sigc::signal<void>& signal_valueChanged() override
	{
		return _sigValueChanged;
	}
};

// Boolean specialisation, making use of the contained getFloatValue()
// casting it to a bool (which is true if the float value is not 0.0f).
template<>
class TypedExpression<bool> :
	public IGuiExpression<bool>
{
private:
	GuiExpressionPtr _contained;

	sigc::signal<void> _sigValueChanged;

public:
	TypedExpression(const GuiExpressionPtr& contained) :
		_contained(contained)
	{
		if (_contained)
		{
			// Connect the changed signal of the contained expression
			// to fire our own signal
			_contained->signal_valueChanged().connect([this]()
			{
				signal_valueChanged().emit();
			});
		}
	}

	virtual bool evaluate() override
	{
		return _contained->getFloatValue() != 0.0f;
	}

	sigc::signal<void>& signal_valueChanged() override
	{
		return _sigValueChanged;
	}
};

// An expression representing a Vector4 value
// Translating between a quadruple of script expressions and IGuiExpression<Vector4>
class Vector4Expression :
	public IGuiExpression<Vector4>
{
private:
	std::vector<GuiExpressionPtr> _vec;

	sigc::signal<void> _sigValueChanged;

public:
	Vector4Expression(const GuiExpressionPtr& x, const GuiExpressionPtr& y, 
					  const GuiExpressionPtr& z, const GuiExpressionPtr& w) :
		_vec(4) // initialise to size 4
	{
		_vec[0] = x;
		_vec[1] = y;
		_vec[2] = z;
		_vec[3] = w;

		for (const GuiExpressionPtr& comp : _vec)
		{
			if (!comp) continue;

			comp->signal_valueChanged().connect([this]()
			{
				signal_valueChanged().emit();
			});
		}
	}

	virtual Vector4 evaluate() override
	{
		return Vector4(_vec[0]->getFloatValue(), _vec[1]->getFloatValue(), 
			_vec[2]->getFloatValue(), _vec[3]->getFloatValue());
	}

	sigc::signal<void>& signal_valueChanged() override
	{
		return _sigValueChanged;
	}
};

class GuiStateVariableExpression :
	public GuiExpression
{
private:
	IGui& _gui;
	std::string _variableName;
	
public:
	GuiStateVariableExpression(IGui& gui, const std::string& variableName);

	virtual float getFloatValue() override;
	virtual std::string getStringValue() override;
};

}

