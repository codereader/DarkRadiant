#pragma once

#include "ibrush.h"
#include "ipatch.h"
#include "iundo.h"
#include "math/Vector3.h"
#include "math/Plane3.h"
#include "render/TexCoord2f.h"
#include "math/AABB.h"
#include "scenelib.h"

namespace test
{

namespace algorithm
{

// Creates a cubic brush with dimensions 64x64x64 at the given origin
inline scene::INodePtr createCubicBrush(const scene::INodePtr& parent, 
    const Vector3& origin = Vector3(0,0,0),
    const std::string& material = "_default")
{
    UndoableCommand cmd("createBrush");

    auto brushNode = GlobalBrushCreator().createBrush();
    scene::addNodeToContainer(brushNode, parent);

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
    UndoableCommand cmd("createBrush");

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
    for (auto i = 0; i < brush->getNumFaces(); ++i)
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

inline Vector2 getFaceCentroid(const IFace* face)
{
    if (face->getWinding().empty()) return { 0, 0 };

    Vector2 centroid = face->getWinding()[0].texcoord;

    for (std::size_t i = 1; i < face->getWinding().size(); ++i)
    {
        centroid += face->getWinding()[i].texcoord;
    }

    centroid /= static_cast<Vector2::ElementType>(face->getWinding().size());

    return centroid;
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

inline bool faceHasVertex(const IFace* face, const Vector3& expectedXYZ, const Vector2& expectedUV)
{
    return algorithm::faceHasVertex(face, [&](const WindingVertex& vertex)
    {
        return math::isNear(vertex.vertex, expectedXYZ, 0.01) && math::isNear(vertex.texcoord, expectedUV, 0.01);
    });
}

inline void foreachPatchVertex(const IPatch& patch, const std::function<void(const PatchControl&)>& functor)
{
    for (std::size_t col = 0; col < patch.getWidth(); ++col)
    {
        for (std::size_t row = 0; row < patch.getHeight(); ++row)
        {
            functor(patch.ctrlAt(row, col));
        }
    }
}

inline bool patchHasVertex(const IPatch& patch, const Vector3& position)
{
    for (std::size_t col = 0; col < patch.getWidth(); ++col)
    {
        for (std::size_t row = 0; row < patch.getHeight(); ++row)
        {
            if (math::isNear(patch.ctrlAt(row, col).vertex, position, 0.01))
            {
                return true;
            }
        }
    }

    return false;
}

inline bool patchHasVertices(const IPatch& patch, const std::vector<Vector3>& positions)
{
    bool result = patch.getWidth() > 0 && patch.getHeight() > 0;

    for (const auto& position : positions)
    {
        if (!patchHasVertex(patch, position))
        {
            result = false;
            break;
        }
    }

    return result;
}

inline AABB getTextureSpaceBounds(const IPatch& patch)
{
    AABB bounds;

    foreachPatchVertex(patch, [&](const PatchControl& control)
    {
        const auto& uv = control.texcoord;
        bounds.includePoint({ uv.x(), uv.y(), 0 });
    });

    return bounds;
}

inline AABB getTextureSpaceBounds(const IFace& face)
{
    AABB bounds;

    for (const auto& vertex : face.getWinding())
    {
        bounds.includePoint({ vertex.texcoord.x(), vertex.texcoord.y(), 0 });
    }

    return bounds;
}

inline void expectVerticesHaveBeenFlipped(int axis, const IPatch& patch, const std::vector<Vector2>& oldTexcoords, const Vector2& flipCenter)
{
    auto old = oldTexcoords.begin();
    algorithm::foreachPatchVertex(patch, [&](const PatchControl& ctrl)
    {
        // Calculate the mirrored coordinate
        auto expectedTexcoord = *(old++);
        expectedTexcoord[axis] = 2 * flipCenter[axis] - expectedTexcoord[axis];

        EXPECT_EQ(ctrl.texcoord.x(), expectedTexcoord.x()) << "Mirrored vertex should be at " << expectedTexcoord;
        EXPECT_EQ(ctrl.texcoord.y(), expectedTexcoord.y()) << "Mirrored vertex should be at " << expectedTexcoord;
    });
}

inline void foreachFace(IBrush& brush, const std::function<void(IFace&)>& functor)
{
    for (int i = 0; i < brush.getNumFaces(); ++i)
    {
        functor(brush.getFace(i));
    }
}

}

}
