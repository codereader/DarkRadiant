#pragma once

#include "iglprogram.h"
#include "igl.h"
#include "math/Matrix4.h"

namespace render
{

/// Implementation of GLProgram
class GLSLProgramBase: public GLProgram
{
protected:
    // Program object
    GLuint _programObj = 0;

    void loadMatrixUniform(GLuint location, const Matrix4& matrix);
    void loadTextureMatrixUniform(GLuint location, const Matrix4& matrix);

public:
    ~GLSLProgramBase();

    // Partial GLProgram implementation
    void enable() override;
    void disable() override;
};

}
