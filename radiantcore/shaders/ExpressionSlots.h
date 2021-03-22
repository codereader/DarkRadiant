#pragma once

#include "ishaderlayer.h"
#include "ishaderexpression.h"

namespace shaders
{

struct ExpressionSlot
{
    // The register holding the evaluated float
    std::size_t registerIndex;

    // The expression itself (empty if unused)
    IShaderExpression::Ptr expression;

    ExpressionSlot() :
        registerIndex(REG_ZERO)
    {}
};

class ExpressionSlots :
    public std::vector<ExpressionSlot>
{
private:
    Registers& _registers;

    static const IShaderExpression::Ptr NullExpression;

public:
    ExpressionSlots(Registers& registers);

    // Copy ctor
    ExpressionSlots(const ExpressionSlots& other, Registers& registers);

    // Store the given expression in the slot
    void assign(IShaderLayer::Expression::Slot slot, const IShaderExpression::Ptr& expression, std::size_t defaultRegisterIndex);

    // Try to parse the given expression string and store it in the slot
    // An empty expression will clear the slot, setting it back to the defaultRegisterIndex
    // Parsing failures of non-empty strings will not change the slot
    void assignFromString(IShaderLayer::Expression::Slot slot, const std::string& expression, std::size_t defaultRegisterIndex);

private:
    // Returns true if the given register index is in use by more than one expression
    bool registerIsShared(std::size_t index) const;
};

}
