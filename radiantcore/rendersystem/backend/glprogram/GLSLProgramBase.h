#pragma once

#include "iglprogram.h"
#include "igl.h"

namespace render
{

class GLSLProgramBase :
    public GLProgram
{
protected:
    // Program object
    GLuint _programObj;

    GLSLProgramBase() :
        _programObj(0)
    {}

public:
    // Partial GLProgram implementation
    virtual void destroy() override;
    virtual void enable() override;
    virtual void disable() override;
};

}
