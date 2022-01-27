#pragma once

#include <map>
#include <sigc++/connection.h>
#include <sigc++/trackable.h>
#include <sigc++/functors/mem_fun.h>
#include "irender.h"
#include "itextstream.h"

namespace entity
{

class RenderableSurfaceCollection :
    public sigc::trackable
{
private:
    AABB _collectionBounds;
    bool _collectionBoundsNeedUpdate;

    std::map<render::IRenderableSurface::Ptr, sigc::connection> _surfaces;

public:
    RenderableSurfaceCollection() :
        _collectionBoundsNeedUpdate(true)
    {}

    void addSurface(const render::IRenderableSurface::Ptr& surface)
    {
        sigc::connection subscription = surface->signal_boundsChanged().connect(
            sigc::mem_fun(*this, &RenderableSurfaceCollection::onSurfaceBoundsChanged));
        
        if (!_surfaces.try_emplace(surface, subscription).second)
        {
            // We've already been subscribed to this one
            subscription.disconnect();
            rWarning() << "Renderable surface has already been attached to entity" << std::endl;
            return;
        }

        _collectionBoundsNeedUpdate = true;
    }

    void removeSurface(const render::IRenderableSurface::Ptr& surface)
    {
        auto mapping = _surfaces.find(surface);

        if (mapping != _surfaces.end())
        {
            mapping->second.disconnect();
            _surfaces.erase(mapping);
        }
        else
        {
            rWarning() << "Renderable surface has not been attached to entity" << std::endl;
        }

        _collectionBoundsNeedUpdate = true;
    }

    void foreachSurfaceTouchingBounds(const AABB& bounds,
        const std::function<void(const render::IRenderableSurface::Ptr&)>& functor)
    {
        if (_surfaces.empty()) return;

        ensureBoundsUpToDate();

        // If the whole collection doesn't intersect, quit early
        if (!_collectionBounds.intersects(bounds)) return;
        
        for (const auto& [surface, _] : _surfaces)
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
    void onSurfaceBoundsChanged()
    {
        _collectionBoundsNeedUpdate = true;
    }

    void ensureBoundsUpToDate()
    {
        if (!_collectionBoundsNeedUpdate) return;
        
        _collectionBoundsNeedUpdate = false;

        _collectionBounds = AABB();

        for (const auto& [surface, _] : _surfaces)
        {
            _collectionBounds.includeAABB(AABB::createFromOrientedAABBSafe(
                surface->getSurfaceBounds(), surface->getSurfaceTransform()));
        }
    }
};

}
