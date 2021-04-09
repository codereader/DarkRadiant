#include "GLSLProgramBase.h"

#include "itextstream.h"
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

}
