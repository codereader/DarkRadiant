#include "GLSLDepthFillProgram.h"
#include "render/backend/GLProgramFactory.h"

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
    std::cout << "[renderer] Creating GLSL depthfill program" << std::endl;
    _programObj = GLProgramFactory::createGLSLProgram(
        DEPTHFILL_VP_FILENAME, DEPTHFILL_FP_FILENAME
    );

    GlobalOpenGL_debugAssertNoErrors();
}

void GLSLDepthFillProgram::destroy()
{
    glDeleteProgram(_programObj);

    GlobalOpenGL_debugAssertNoErrors();
}

void GLSLDepthFillProgram::enable()
{
    GlobalOpenGL_debugAssertNoErrors();

    assert(glIsProgram(_programObj));
    glUseProgram(_programObj);

    GlobalOpenGL_debugAssertNoErrors();
}

void GLSLDepthFillProgram::disable()
{
    glUseProgram(0);

    GlobalOpenGL_debugAssertNoErrors();
}

}

