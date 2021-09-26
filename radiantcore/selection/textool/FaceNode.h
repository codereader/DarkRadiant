#pragma once

#include "math/AABB.h"
#include "NodeBase.h"

class Matrix3;
class IFace;

namespace textool
{

/**
 * Represents a single face in the Texture Tool scene.
 */
class FaceNode final :
    public NodeBase,
    public IFaceNode
{
private:
    IFace& _face;
    mutable AABB _bounds;

public:
    FaceNode(IFace& face);

    IFace& getFace() override;

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
    void transformSelectedAndRecalculateTexDef(const std::function<void(Vector2&)>& transform);
};

}
