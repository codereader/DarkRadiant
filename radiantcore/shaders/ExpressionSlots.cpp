#include "ExpressionSlots.h"

#include "ShaderExpression.h"

namespace shaders
{

ExpressionSlots::ExpressionSlots(Registers& registers) :
    _registers(registers)
{
    resize(IShaderLayer::Expression::NumExpressionSlots);
}

ExpressionSlots::ExpressionSlots(const ExpressionSlots& other, Registers& registers) :
    std::vector<ExpressionSlot>(other.size()),
    _registers(registers)
{
    for (auto i = 0; i < other.size(); ++i)
    {
        auto& thisSlot = at(i);
        auto& otherSlot = other.at(i);

        thisSlot.registerIndex = otherSlot.registerIndex;

        if (otherSlot.expression)
        {
            thisSlot.expression = otherSlot.expression->clone();
            thisSlot.expression->linkToSpecificRegister(_registers, thisSlot.registerIndex);
        }
    }
}

void ExpressionSlots::assign(IShaderLayer::Expression::Slot slot, const IShaderExpression::Ptr& newExpression, std::size_t defaultRegisterIndex)
{
    auto& expressionSlot = at(slot);

    if (!newExpression)
    {
        expressionSlot.expression.reset();
        expressionSlot.registerIndex = defaultRegisterIndex;
        return;
    }

    // Non-empty expression, overwrite if we have an existing expression in the slot
    // Beware of the fact that some expressions could be shared across slots, before re-using the same register
    if (expressionSlot.expression && !registerIsShared(expressionSlot.registerIndex))
    {
        // We assume that if there was an expression in the slot, it shouldn't point to the default registers
        assert(expressionSlot.registerIndex != defaultRegisterIndex);

        // Re-use the register index
        expressionSlot.expression = newExpression;
        expressionSlot.expression->linkToSpecificRegister(_registers, expressionSlot.registerIndex);
    }
    else
    {
        expressionSlot.expression = newExpression;
        expressionSlot.registerIndex = expressionSlot.expression->linkToRegister(_registers);
    }
}

void ExpressionSlots::assignFromString(IShaderLayer::Expression::Slot slot, const std::string& expressionString, std::size_t defaultRegisterIndex)
{
    // An empty string will clear the expression
    if (expressionString.empty())
    {
        assign(slot, IShaderExpression::Ptr(), defaultRegisterIndex);
        return;
    }

    // Attempt to parse the string
    auto expression = ShaderExpression::createFromString(expressionString);

    if (!expression)
    {
        return; // parsing failures will not overwrite the expression slot
    }

    assign(slot, expression, defaultRegisterIndex);
}

bool ExpressionSlots::registerIsShared(std::size_t index) const
{
    std::size_t useCount = 0;

    for (const auto& slot : *this)
    {
        if (slot.registerIndex == index && ++useCount > 1)
        {
            return true;
        }
    }

    return false;
}

}
