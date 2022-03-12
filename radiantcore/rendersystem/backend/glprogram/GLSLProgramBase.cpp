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

}
