#pragma once

#include <set>
#include "irender.h"

namespace entity
{

class RenderableSurfaceCollection
{
private:
    AABB _collectionBounds;
    bool _collectionBoundsNeedUpdate;

    std::set<render::IRenderableSurface::Ptr> _surfaces;

public:
    RenderableSurfaceCollection() :
        _collectionBoundsNeedUpdate(true)
    {}

    void addSurface(const render::IRenderableSurface::Ptr& surface)
    {
        _surfaces.insert(surface);
        _collectionBoundsNeedUpdate = true;
    }

    void removeSurface(const render::IRenderableSurface::Ptr& surface)
    {
        _surfaces.erase(surface);
        _collectionBoundsNeedUpdate = true;
    }

    void foreachSurfaceTouchingBounds(const AABB& bounds,
        const std::function<void(const render::IRenderableSurface::Ptr&)>& functor)
    {
        if (_surfaces.empty()) return;

        ensureBoundsUpToDate();

        // If the whole collection doesn't intersect, quit early
        // TODO: bounds not updated when surface changes
        //if (!_collectionBounds.intersects(bounds)) return;
        
        for (const auto& surface : _surfaces)
        {
            auto orientedBounds = AABB::createFromOrientedAABBSafe(
                surface->getSurfaceBounds(), surface->getSurfaceTransform());

            if (bounds.intersects(orientedBounds))
            {
                functor(surface);
            }
        }
    }

private:
    void ensureBoundsUpToDate()
    {
        if (!_collectionBoundsNeedUpdate) return;
        
        _collectionBoundsNeedUpdate = false;

        _collectionBounds = AABB();

        for (const auto& surface : _surfaces)
        {
            _collectionBounds.includeAABB(AABB::createFromOrientedAABBSafe(
                surface->getSurfaceBounds(), surface->getSurfaceTransform()));
        }
    }
};

}
