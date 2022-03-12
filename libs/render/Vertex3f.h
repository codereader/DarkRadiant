#pragma once

#include "VertexTraits.h"

#include "math/Vector3.h"

using Vertex3 = Vector3;
using Normal3 = Vertex3;

namespace render
{

/// VertexTraits for Vertex3
template<> class VertexTraits<Vertex3>
{
public:
    static const void* VERTEX_OFFSET()
    {
        return 0;
    }

    static bool hasNormal() { return false; }
    static const void* NORMAL_OFFSET() { return 0; }

    static bool hasTexCoord() { return false; }
    static const void* TEXCOORD_OFFSET() { return 0; }

    static bool hasTangents() { return false; }
    static const void* TANGENT_OFFSET() { return 0; }
    static const void* BITANGENT_OFFSET() { return 0; }
};

}

