#pragma once

#include "ipatch.h"
#include "NodeBase.h"

namespace textool
{

class PatchNode :
    public NodeBase
{
private:
    IPatch& _patch;
    mutable AABB _bounds;

public:
    PatchNode(IPatch& patch) :
        _patch(patch)
    {}

    void applyTransformToSelected(const Matrix3& transform) override
    {
        // TODO
    }

    const AABB& localAABB() const
    {
        _bounds = AABB();

        for (std::size_t col = 0; col < _patch.getWidth(); ++col)
        {
            for (std::size_t row = 0; row < _patch.getHeight(); ++row)
            {
                const auto& ctrl = _patch.ctrlAt(row, col);
                _bounds.includePoint({ ctrl.texcoord.x(), ctrl.texcoord.y(), 0 });
            }
        }

        return _bounds;
    }

    void testSelect(Selector& selector, SelectionTest& test) override
    {

    }
};

}
