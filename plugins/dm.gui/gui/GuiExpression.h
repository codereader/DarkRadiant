#pragma once

#include <memory>
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

// An expression representing a constant floating point number
class ConstantExpression :
	public GuiExpression
{
private:
	float _floatValue;
	std::string _stringValue;

public:
	ConstantExpression(const std::string& stringValue);

	explicit ConstantExpression(float value);

	virtual float getFloatValue();
	virtual std::string getStringValue();
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

