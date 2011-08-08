#pragma once

#include "ishaders.h"
#include "parser/Tokeniser.h"

namespace shaders
{

class ShaderExpression :
	public IShaderExpression
{
public:
	static IShaderExpressionPtr createFromString(const std::string& exprStr);

private:
	// Recursive parsing method
	static IShaderExpressionPtr createFromTokens(parser::StringTokeniser& tokeniser);
};

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

} // namespace
