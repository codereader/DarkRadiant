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
    Registers& _registers;

    struct TemporaryMatrix
    {
        IShaderExpression::Ptr xx;
        IShaderExpression::Ptr yx;
        IShaderExpression::Ptr tx;
        IShaderExpression::Ptr xy;
        IShaderExpression::Ptr yy;
        IShaderExpression::Ptr ty;
    };

public:
    TextureMatrix(ExpressionSlots& expressions, Registers& registers) :
        _expressions(expressions),
        _registers(registers)
    {}

    TextureMatrix(const TextureMatrix& other) = delete;

    void setIdentity()
    {
        // Initialise the texture matrix to identity (set the diagonals to 1)
        xx().registerIndex = REG_ONE;
        yx().registerIndex = REG_ZERO;
        tx().registerIndex = REG_ZERO;
        xy().registerIndex = REG_ZERO;
        yy().registerIndex = REG_ONE;
        ty().registerIndex = REG_ZERO;
    }

    Matrix4 getMatrix4()
    {
        auto matrix = Matrix4::getIdentity();

        matrix.xx() = _registers[xx().registerIndex];
        matrix.yx() = _registers[yx().registerIndex];
        matrix.tx() = _registers[tx().registerIndex];
        matrix.xy() = _registers[xy().registerIndex];
        matrix.yy() = _registers[yy().registerIndex];
        matrix.ty() = _registers[ty().registerIndex];

        return matrix;
    }

    void applyTransformation(const IShaderLayer::Transformation& transformation)
    {
        TemporaryMatrix matrix;

        switch (transformation.type)
        {
        case IShaderLayer::TransformType::Translate:
            matrix.xx = ShaderExpression::createConstant(1);
            matrix.yx = ShaderExpression::createConstant(0);
            matrix.tx = transformation.expression1;
            matrix.xy = ShaderExpression::createConstant(0);
            matrix.yy = ShaderExpression::createConstant(1);
            matrix.ty = transformation.expression2;
            break;
        case IShaderLayer::TransformType::Scale:
            matrix.xx = transformation.expression1;
            matrix.yx = ShaderExpression::createConstant(0);
            matrix.tx = ShaderExpression::createConstant(0);
            matrix.xy = ShaderExpression::createConstant(0);
            matrix.yy = transformation.expression2;
            matrix.ty = ShaderExpression::createConstant(0);
            break;
        case IShaderLayer::TransformType::CenterScale:
            matrix.xx = transformation.expression1;
            matrix.yx = ShaderExpression::createConstant(0);
            matrix.tx = ShaderExpression::createAddition(
                ShaderExpression::createConstant(0.5),
                ShaderExpression::createMultiplication(ShaderExpression::createConstant(-0.5), transformation.expression1)
            );
            matrix.xy = ShaderExpression::createConstant(0);
            matrix.yy = transformation.expression2;
            matrix.ty = ShaderExpression::createAddition(
                ShaderExpression::createConstant(0.5),
                ShaderExpression::createMultiplication(ShaderExpression::createConstant(-0.5), transformation.expression2)
            );
            break;
        case IShaderLayer::TransformType::Shear:
            matrix.xx = ShaderExpression::createConstant(1);
            matrix.yx = transformation.expression1;
            matrix.tx = ShaderExpression::createMultiplication(ShaderExpression::createConstant(-0.5), transformation.expression1);
            matrix.xy = transformation.expression2;
            matrix.yy = ShaderExpression::createConstant(1);
            matrix.ty = ShaderExpression::createMultiplication(ShaderExpression::createConstant(-0.5), transformation.expression2);
            break;
        default:
            return;
        };

        multiplyMatrix(matrix);
    }

private:
    void multiplyMatrix(const TemporaryMatrix& matrix)
    {
        auto xx = add(multiply(matrix.xx, this->xx()), multiply(matrix.yx, this->xy()));
        auto xy = add(multiply(matrix.xy, this->xx()), multiply(matrix.yy, this->xy()));
        auto yx = add(multiply(matrix.xx, this->yx()), multiply(matrix.yx, this->yy()));
        auto yy = add(multiply(matrix.xy, this->yx()), multiply(matrix.yy, this->yy()));
        auto tx = add(add(multiply(matrix.xx, this->tx()), multiply(matrix.yx, this->ty())), matrix.tx);
        auto ty = add(add(multiply(matrix.xy, this->tx()), multiply(matrix.yy, this->ty())), matrix.ty);

        _expressions.assign(IShaderLayer::Expression::TextureMatrixRow0Col0, xx, REG_ONE);
        _expressions.assign(IShaderLayer::Expression::TextureMatrixRow0Col1, yx, REG_ZERO);
        _expressions.assign(IShaderLayer::Expression::TextureMatrixRow1Col1, yy, REG_ONE);
        _expressions.assign(IShaderLayer::Expression::TextureMatrixRow1Col0, xy, REG_ZERO);
        _expressions.assign(IShaderLayer::Expression::TextureMatrixRow0Col2, tx, REG_ZERO);
        _expressions.assign(IShaderLayer::Expression::TextureMatrixRow1Col2, ty, REG_ZERO);
    }

    IShaderExpression::Ptr add(const IShaderExpression::Ptr& a, const IShaderExpression::Ptr& b)
    {
        return ShaderExpression::createAddition(a, b);
    }

    IShaderExpression::Ptr multiply(const IShaderExpression::Ptr& a, ExpressionSlot& b)
    {
        // Create a constant if there's no expression in the slot yet
        auto bExpr = b.expression ? b.expression : ShaderExpression::createConstant(_registers[b.registerIndex]);

        return ShaderExpression::createMultiplication(a, bExpr);
    }

    // Shortcut accessors
    ExpressionSlot& xx() { return _expressions[IShaderLayer::Expression::TextureMatrixRow0Col0]; }
    ExpressionSlot& yx() { return _expressions[IShaderLayer::Expression::TextureMatrixRow0Col1]; }
    ExpressionSlot& yy() { return _expressions[IShaderLayer::Expression::TextureMatrixRow1Col1]; }
    ExpressionSlot& xy() { return _expressions[IShaderLayer::Expression::TextureMatrixRow1Col0]; }
    ExpressionSlot& tx() { return _expressions[IShaderLayer::Expression::TextureMatrixRow0Col2]; }
    ExpressionSlot& ty() { return _expressions[IShaderLayer::Expression::TextureMatrixRow1Col2]; }
};

}
