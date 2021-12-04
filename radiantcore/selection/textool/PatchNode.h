#pragma once

#include <functional>
#include "math/AABB.h"
#include "Node.h"

class IPatch;
struct PatchControl;

namespace textool
{

class PatchNode final :
    public Node,
    public IPatchNode
{
private:
    IPatch& _patch;
    mutable AABB _bounds;

public:
    PatchNode(IPatch& patch);

    IPatch& getPatch() override;

    void beginTransformation() override;
    void revertTransformation() override;
    void commitTransformation() override;

    void transform(const Matrix3& transform) override;
    void transformComponents(const Matrix3& transform) override;

    const AABB& localAABB() const override;
    void testSelect(Selector& selector, SelectionTest& test) override;

    void render(SelectionMode mode) override;

    void expandSelectionToRelated() override;
    void snapto(float snap) override;
    void snapComponents(float snap) override;
    void mergeComponentsWith(const Vector2& center) override;

private:
    void foreachVertex(const std::function<void(PatchControl&)>& functor) const;
};

}
