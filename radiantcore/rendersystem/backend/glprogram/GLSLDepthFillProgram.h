#pragma once

#include "iglprogram.h"
#include "igl.h"

namespace render
{

class GLSLDepthFillProgram :
	public GLProgram
{
    // Program object
    GLuint _programObj;

public:

    /* GLProgram implementation */
    void create();
    void destroy();
    void enable();
    void disable();
};

} // namespace render

