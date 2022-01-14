#pragma once

#include "imodelsurface.h"
#include "render/RenderableSurface.h"

namespace model
{

// Wraps a StaticModelSurface to implement the IRenderableSurface interface
// required to draw an oriented mesh in the scene.
class RenderableStaticSurface final :
    public render::RenderableSurface
{
private:
    const IIndexedModelSurface& _surface;
    const Matrix4& _localToWorld;

public:
    using Ptr = std::shared_ptr<RenderableStaticSurface>;

    // Construct this renderable around the existing surface.
    // The reference to the orientation matrix is stored and needs to remain valid
    RenderableStaticSurface(const IIndexedModelSurface& surface, const Matrix4& localToWorld) :
        _surface(surface),
        _localToWorld(localToWorld)
    {}

    RenderableStaticSurface(const RenderableStaticSurface& other) = delete;
    RenderableStaticSurface& operator=(const RenderableStaticSurface& other) = delete;

    const IIndexedModelSurface& getSurface() const
    {
        return _surface;
    }

    const std::vector<ArbitraryMeshVertex>& getVertices() override
    {
        return _surface.getVertexArray();
    }

    const std::vector<unsigned int>& getIndices() override
    {
        return _surface.getIndexArray();
    }

    const Matrix4& getSurfaceTransform() override
    {
        return _localToWorld;
    }

    const AABB& getSurfaceBounds() override
    {
        return _surface.getSurfaceBounds();
    }
};

}
