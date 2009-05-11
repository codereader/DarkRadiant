#include "ARBDepthFillProgram.h"
#include "render/backend/GLProgramFactory.h"

namespace render
{

/* CONSTANTS */
namespace {

#ifdef RADIANT_USE_GLSL
    const char* DEPTHFILL_VP_FILENAME = "zfill_vp.glsl";
    const char* DEPTHFILL_FP_FILENAME = "zfill_fp.glsl";
#else
    const char* DEPTHFILL_VP_FILENAME = "zfill_vp.glp";
    const char* DEPTHFILL_FP_FILENAME = "zfill_fp.glp";
#endif

}

void ARBDepthFillProgram::create()
{
#ifdef RADIANT_USE_GLSL

    // Create the program object
    std::cout << "[renderer] Creating GLSL depthfill program" << std::endl;
    _programObj = GLProgramFactory::createGLSLProgram(
        DEPTHFILL_VP_FILENAME, DEPTHFILL_FP_FILENAME
    );

#else

    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);

    // Create the vertex program
    m_vertex_program = GLProgramFactory::createARBProgram(
        DEPTHFILL_VP_FILENAME, GL_VERTEX_PROGRAM_ARB
    );
    
    // Create the fragment program
    m_fragment_program = GLProgramFactory::createARBProgram(
        DEPTHFILL_FP_FILENAME, GL_FRAGMENT_PROGRAM_ARB
    );

    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);

#endif

    GlobalOpenGL_debugAssertNoErrors();
}

void ARBDepthFillProgram::destroy()
{
#ifdef RADIANT_USE_GLSL
    glDeleteProgram(_programObj);
#else
    glDeleteProgramsARB(1, &m_vertex_program);
    glDeleteProgramsARB(1, &m_fragment_program);
#endif

    GlobalOpenGL_debugAssertNoErrors();
}

void ARBDepthFillProgram::enable()
{
    GlobalOpenGL_debugAssertNoErrors();

#ifdef RADIANT_USE_GLSL
    assert(glIsProgram(_programObj));
    glUseProgram(_programObj);
#else
    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_vertex_program);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_fragment_program);
#endif

    GlobalOpenGL_debugAssertNoErrors();
}

void ARBDepthFillProgram::disable()
{
#ifdef RADIANT_USE_GLSL
    glUseProgram(0);
#else
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
#endif

    GlobalOpenGL_debugAssertNoErrors();
}

}

