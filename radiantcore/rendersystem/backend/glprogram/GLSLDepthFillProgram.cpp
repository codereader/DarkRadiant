#include "GLSLDepthFillProgram.h"
#include "../GLProgramFactory.h"

#include "itextstream.h"

namespace render
{

/* CONSTANTS */
namespace {

    const char* DEPTHFILL_VP_FILENAME = "zfill_vp.glsl";
    const char* DEPTHFILL_FP_FILENAME = "zfill_fp.glsl";
}

void GLSLDepthFillProgram::create()
{
    // Create the program object
    rConsole() << "[renderer] Creating GLSL depthfill program" << std::endl;

    _programObj = GLProgramFactory::createGLSLProgram(
        DEPTHFILL_VP_FILENAME, DEPTHFILL_FP_FILENAME
    );

    debug::assertNoGlErrors();
}

void GLSLDepthFillProgram::destroy()
{
    glDeleteProgram(_programObj);

    debug::assertNoGlErrors();
}

void GLSLDepthFillProgram::enable()
{
    debug::assertNoGlErrors();

    assert(glIsProgram(_programObj));
    glUseProgram(_programObj);

    debug::assertNoGlErrors();
}

void GLSLDepthFillProgram::disable()
{
    glUseProgram(0);

    debug::assertNoGlErrors();
}

}

