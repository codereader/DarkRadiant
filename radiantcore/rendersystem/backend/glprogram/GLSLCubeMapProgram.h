#pragma once

#include "GLProgramAttributes.h"
#include "GLSLProgramBase.h"

namespace render
{

class GLSLCubeMapProgram :
    public GLSLProgramBase
{
private:
    // Uniform/program-local parameter IDs.
    GLint _locViewOrigin;

public:
    GLSLCubeMapProgram() :
        _locViewOrigin(-1)
    {}

    /* GLProgram implementation */
    void create() override;
    void enable() override;
    void disable() override;

    void applyRenderParams(const Vector3& viewer,
        const Matrix4& localToWorld,
        const Params&);
};

} // namespace render

