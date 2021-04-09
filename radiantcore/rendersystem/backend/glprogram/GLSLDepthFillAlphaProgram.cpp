#include "GLSLDepthFillAlphaProgram.h"

#include "../GLProgramFactory.h"
#include "debugging/gl.h"

#include "itextstream.h"

namespace render
{

namespace
{
    const char* DEPTHFILL_ALPHA_VP_FILENAME = "zfill_vp.glsl"; // use the same VP
    const char* DEPTHFILL_ALPHA_FP_FILENAME = "zfill_alpha_fp.glsl";
}

void GLSLDepthFillAlphaProgram::create()
{
    // Create the program object
    rMessage() << "[renderer] Creating GLSL depthfill+alpha program" << std::endl;

    _programObj = GLProgramFactory::createGLSLProgram(
        DEPTHFILL_ALPHA_VP_FILENAME, DEPTHFILL_ALPHA_FP_FILENAME
    );

    debug::assertNoGlErrors();
}

}

