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

}

