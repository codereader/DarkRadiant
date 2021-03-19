#pragma once

#include "itextstream.h"
#include "ExpressionSlots.h"

namespace shaders
{

/**
 * Encapsulates the texture matrix handling, connecting and nesting 
 * the expressions that are used to calculate the matrix components.
 * 
 * Only the 6 relevant components of the texture matrix are linked to 
 * registers and possibly expressions:
 * 
 * | m00  m01  0  m02 |
 * | m10  m11  0  m12 |
 * |  0    0   1   0  |
 * |  0    0   0   1  |
 */
class TextureMatrix
{
private:
    ExpressionSlots& _expressions;
    Registers& _registers;

    struct TemporaryMatrix;

public:
    TextureMatrix(ExpressionSlots& expressions, Registers& registers);

    TextureMatrix(const TextureMatrix& other) = delete;

    // Sets the expression slot bindings such that the matrix will evaluate to identity
    void setIdentity();

    // Return the matrix as stored in the registers
    Matrix4 getMatrix4();

    // pre-multiply the given transformation matrix to the existing one
    void applyTransformation(const IShaderLayer::Transformation& transformation);

private:
    void multiplyMatrix(const TemporaryMatrix& matrix);

    IShaderExpression::Ptr add(const IShaderExpression::Ptr& a, const IShaderExpression::Ptr& b);
    IShaderExpression::Ptr multiply(const IShaderExpression::Ptr& a, ExpressionSlot& b);

    // Shortcut accessors
    ExpressionSlot& xx() { return _expressions[IShaderLayer::Expression::TextureMatrixRow0Col0]; }
    ExpressionSlot& yx() { return _expressions[IShaderLayer::Expression::TextureMatrixRow0Col1]; }
    ExpressionSlot& yy() { return _expressions[IShaderLayer::Expression::TextureMatrixRow1Col1]; }
    ExpressionSlot& xy() { return _expressions[IShaderLayer::Expression::TextureMatrixRow1Col0]; }
    ExpressionSlot& tx() { return _expressions[IShaderLayer::Expression::TextureMatrixRow0Col2]; }
    ExpressionSlot& ty() { return _expressions[IShaderLayer::Expression::TextureMatrixRow1Col2]; }
};

}
