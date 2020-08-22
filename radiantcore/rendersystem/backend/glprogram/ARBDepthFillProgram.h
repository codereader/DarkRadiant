#pragma once

#include "iglprogram.h"
#include "igl.h"

namespace render {

class ARBDepthFillProgram :
	public GLProgram
{
    GLuint m_vertex_program;
    GLuint m_fragment_program;

public:

    /* GLProgram implementation */
    void create();
    void destroy();
    void enable();
    void disable();
};

} // namespace render

