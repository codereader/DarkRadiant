#include "GLSLDepthFillProgram.h"
#include "../GLProgramFactory.h"
#include "debugging/gl.h"

#include "itextstream.h"

namespace render
{

namespace 
{
    const char* DEPTHFILL_VP_FILENAME = "zfill_vp.glsl";
    const char* DEPTHFILL_FP_FILENAME = "zfill_fp.glsl";
}

void GLSLDepthFillProgram::create()
{
    // Create the program object
    rMessage() << "[renderer] Creating GLSL depthfill program" << std::endl;

    _programObj = GLProgramFactory::createGLSLProgram(
        DEPTHFILL_VP_FILENAME, DEPTHFILL_FP_FILENAME
    );

    debug::assertNoGlErrors();
}

}

