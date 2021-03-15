#pragma once

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

public:
    TextureMatrix(ExpressionSlots& expressions) :
        _expressions(expressions)
    {}

    TextureMatrix(const TextureMatrix& other) = delete;

    void setIdentity()
    {
        // Initialise the texture matrix to identity (set the diagonals to 1)
        _expressions[IShaderLayer::Expression::TextureMatrixRow0Col0].registerIndex = REG_ONE;
        _expressions[IShaderLayer::Expression::TextureMatrixRow0Col1].registerIndex = REG_ZERO;
        _expressions[IShaderLayer::Expression::TextureMatrixRow0Col2].registerIndex = REG_ZERO;
        _expressions[IShaderLayer::Expression::TextureMatrixRow1Col0].registerIndex = REG_ZERO;
        _expressions[IShaderLayer::Expression::TextureMatrixRow1Col1].registerIndex = REG_ONE;
        _expressions[IShaderLayer::Expression::TextureMatrixRow1Col2].registerIndex = REG_ZERO;
    }

    Matrix4 getMatrix4(const Registers& registers) const
    {
        auto matrix = Matrix4::getIdentity();

        matrix.xx() = registers[_expressions[IShaderLayer::Expression::TextureMatrixRow0Col0].registerIndex];
        matrix.yx() = registers[_expressions[IShaderLayer::Expression::TextureMatrixRow0Col1].registerIndex];
        matrix.yy() = registers[_expressions[IShaderLayer::Expression::TextureMatrixRow1Col1].registerIndex];
        matrix.xy() = registers[_expressions[IShaderLayer::Expression::TextureMatrixRow1Col0].registerIndex];
        matrix.tx() = registers[_expressions[IShaderLayer::Expression::TextureMatrixRow0Col2].registerIndex];
        matrix.ty() = registers[_expressions[IShaderLayer::Expression::TextureMatrixRow1Col2].registerIndex];

        return matrix;
    }

    void applyTransformation(const IShaderLayer::Transformation& transformation)
    {
        // TODO
    }
};

}
