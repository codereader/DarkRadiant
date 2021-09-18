#pragma once

#include "ibrush.h"
#include "math/Plane3.h"
#include "math/AABB.h"

namespace test
{

namespace algorithm
{

// Creates a cubic brush with dimensions 64x64x64 at the given origin
inline scene::INodePtr createCubicBrush(const scene::INodePtr& parent, 
    const Vector3& origin = Vector3(0,0,0),
    const std::string& material = "_default")
{
    auto brushNode = GlobalBrushCreator().createBrush();
    parent->addChildNode(brushNode);

    auto& brush = *Node_getIBrush(brushNode);

    auto translation = Matrix4::getTranslation(origin);
    brush.addFace(Plane3(+1, 0, 0, 64).transform(translation));
    brush.addFace(Plane3(-1, 0, 0, 64).transform(translation));
    brush.addFace(Plane3(0, +1, 0, 64).transform(translation));
    brush.addFace(Plane3(0, -1, 0, 64).transform(translation));
    brush.addFace(Plane3(0, 0, +1, 64).transform(translation));
    brush.addFace(Plane3(0, 0, -1, 64).transform(translation));

    brush.setShader(material);

    brush.evaluateBRep();

    return brushNode;
}

inline scene::INodePtr createCuboidBrush(const scene::INodePtr& parent,
    const AABB& bounds = AABB(Vector3(0, 0, 0), Vector3(64,256,128)),
    const std::string& material = "_default")
{
    auto brushNode = GlobalBrushCreator().createBrush();
    parent->addChildNode(brushNode);

    auto& brush = *Node_getIBrush(brushNode);

    auto translation = Matrix4::getTranslation(-bounds.getOrigin());
    brush.addFace(Plane3(+1, 0, 0, bounds.getExtents().x()).transform(translation));
    brush.addFace(Plane3(-1, 0, 0, bounds.getExtents().x()).transform(translation));
    brush.addFace(Plane3(0, +1, 0, bounds.getExtents().y()).transform(translation));
    brush.addFace(Plane3(0, -1, 0, bounds.getExtents().y()).transform(translation));
    brush.addFace(Plane3(0, 0, +1, bounds.getExtents().z()).transform(translation));
    brush.addFace(Plane3(0, 0, -1, bounds.getExtents().z()).transform(translation));

    brush.setShader(material);

    brush.evaluateBRep();

    return brushNode;
}

inline IFace* findBrushFaceWithNormal(IBrush* brush, const Vector3& normal)
{
    for (auto i = 0; brush->getNumFaces(); ++i)
    {
        auto& face = brush->getFace(i);

        if (math::isNear(face.getPlane3().normal(), normal, 0.01))
        {
            return &face;
        }
    }

    return nullptr;
}

inline bool faceHasVertex(const IFace* face, const std::function<bool(const WindingVertex&)>& predicate)
{
    const auto& winding = face->getWinding();
    
    for (const auto& vertex : winding)
    {
        if (predicate(vertex))
        {
            return true;
        }
    }

    return false;
}

inline scene::INodePtr createPatchFromBounds(const scene::INodePtr& parent,
    const AABB& bounds = AABB(Vector3(0, 0, 0), Vector3(64, 256, 128)),
    const std::string& material = "_default")
{
    auto patchNode = GlobalPatchModule().createPatch(patch::PatchDefType::Def2);
    parent->addChildNode(patchNode);

    auto patch = Node_getIPatch(patchNode);
    patch->setDims(3, 3);
    patch->setShader(material);

    for (std::size_t col = 0; col < patch->getWidth(); ++col)
    {
        auto extents = bounds.getExtents();

        for (std::size_t row = 0; row < patch->getHeight(); ++row)
        {
            patch->ctrlAt(row, col).vertex = bounds.getOrigin() - bounds.getExtents();

            patch->ctrlAt(row, col).vertex.x() += 2 * col * extents.x();
            patch->ctrlAt(row, col).vertex.y() += 2 * row * extents.y();
        }
    }

    patch->controlPointsChanged();

    return patchNode;
}

}

}
