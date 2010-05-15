#pragma once

#include "iglrender.h"

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

    // Required set parameters function, just empty implementation
    void applyRenderParams(const Vector3& viewer,
                           const Matrix4& localToWorld,
                           const Vector3& origin,
                           const Vector3& colour,
                           const Matrix4& world2light,
                           float ambient)
    { }
};

} // namespace render

