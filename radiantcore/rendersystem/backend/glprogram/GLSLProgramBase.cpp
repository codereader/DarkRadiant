#include "GLSLProgramBase.h"

#include "debugging/gl.h"

namespace render
{

void GLSLProgramBase::destroy()
{
    glDeleteProgram(_programObj);
    _programObj = 0;

    debug::assertNoGlErrors();
}

void GLSLProgramBase::enable()
{
    debug::assertNoGlErrors();

    assert(glIsProgram(_programObj));
    glUseProgram(_programObj);

    debug::assertNoGlErrors();
}

void GLSLProgramBase::disable()
{
    glUseProgram(0);

    debug::assertNoGlErrors();
}

void GLSLProgramBase::loadMatrixUniform(GLuint location, const Matrix4& matrix)
{
    float values[16];

    for (auto i = 0; i < 16; ++i)
    {
        values[i] = static_cast<float>(matrix[i]);
    }

    glUniformMatrix4fv(location, 1, GL_FALSE, values);
}

void GLSLProgramBase::loadTextureMatrixUniform(GLuint location, const Matrix4& matrix)
{
    /* Extract the 6 components that are relevant to texture transforms.
     * We load it row-major to be able to use a dot product in GLSL.
     *
     * | m00  m01  0  m02 |
     * | m10  m11  0  m12 |
     * |  0    0   1   0  |
     * |  0    0   0   1  |
     */
    float values[8];

    // First vector is the top row
    values[0] = static_cast<float>(matrix.xx());
    values[1] = static_cast<float>(matrix.yx());
    values[2] = 0.0f;
    values[3] = static_cast<float>(matrix.tx());

    // Second vector is the bottom row
    values[4] = static_cast<float>(matrix.xy());
    values[5] = static_cast<float>(matrix.yy());
    values[6] = 0.0f;
    values[7] = static_cast<float>(matrix.ty());

    // Load two 4-component vectors into the uniform location
    glUniform4fv(location, 2, values);
}

}
