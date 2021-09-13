#pragma once

#include "ibrush.h"
#include "NodeBase.h"

namespace textool
{

class FaceNode :
    public NodeBase
{
private:
    IFace& _face;
    mutable AABB _bounds;

public:
    FaceNode(IFace& face) :
        _face(face)
    {}

    void applyTransformToSelected(const Matrix3& transform) override
    {
        // TODO
    }

    const AABB& localAABB() const
    {
        _bounds = AABB();

        for (const auto& vertex : _face.getWinding())
        {
            _bounds.includePoint({ vertex.texcoord.x(), vertex.texcoord.y(), 0 });
        }

        return _bounds;
    }
};

}
