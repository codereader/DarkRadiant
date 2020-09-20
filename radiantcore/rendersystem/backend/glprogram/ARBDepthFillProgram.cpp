#include "ARBDepthFillProgram.h"
#include "../GLProgramFactory.h"
#include "debugging/gl.h"

namespace render
{

/* CONSTANTS */
namespace {

    const char* DEPTHFILL_VP_FILENAME = "zfill_vp.glp";
    const char* DEPTHFILL_FP_FILENAME = "zfill_fp.glp";

}

void ARBDepthFillProgram::create()
{
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

    debug::assertNoGlErrors();
}

void ARBDepthFillProgram::destroy()
{
    glDeleteProgramsARB(1, &m_vertex_program);
    glDeleteProgramsARB(1, &m_fragment_program);

    debug::assertNoGlErrors();
}

void ARBDepthFillProgram::enable()
{
    debug::assertNoGlErrors();

    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_vertex_program);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_fragment_program);

    debug::assertNoGlErrors();
}

void ARBDepthFillProgram::disable()
{
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);

    debug::assertNoGlErrors();
}

}

