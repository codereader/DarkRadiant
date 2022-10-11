#pragma once

#include "GLSLProgramBase.h"
#include "math/Vector3.h"

namespace render
{

class CubeMapProgram :
    public GLSLProgramBase
{
private:
    // Uniform/program-local parameter IDs.
    GLint _locViewOrigin;

public:
    CubeMapProgram() :
        _locViewOrigin(-1)
    {}

    /* GLProgram implementation */
    void create() override;
    void enable() override;
    void disable() override;

    void setViewer(const Vector3& viewer);
};

} // namespace render

