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

    struct SurfaceData
    {
        ShaderPtr shader;
        sigc::connection boundsChangedConnection;
    };

    std::map<render::IRenderableSurface::Ptr, SurfaceData> _surfaces;

public:
    RenderableSurfaceCollection() :
        _collectionBoundsNeedUpdate(true)
    {}

    void addSurface(const render::IRenderableSurface::Ptr& surface, const ShaderPtr& shader)
    {
        sigc::connection subscription = surface->signal_boundsChanged().connect(
            sigc::mem_fun(*this, &RenderableSurfaceCollection::onSurfaceBoundsChanged));
        
        if (!_surfaces.try_emplace(surface, SurfaceData{ shader, subscription }).second)
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
            mapping->second.boundsChangedConnection.disconnect();
            _surfaces.erase(mapping);
        }
        else
        {
            rWarning() << "Renderable surface has not been attached to entity" << std::endl;
        }

        _collectionBoundsNeedUpdate = true;
    }

    void foreachSurfaceTouchingBounds(const AABB& bounds,
        const IRenderEntity::SurfaceVisitFunction& functor)
    {
        if (_surfaces.empty()) return;

        ensureBoundsUpToDate();

        // If the whole collection doesn't intersect, quit early
        if (!_collectionBounds.intersects(bounds)) return;
        
        for (const auto& [surface, surfaceData] : _surfaces)
        {
            auto orientedBounds = AABB::createFromOrientedAABBSafe(
                surface->getSurfaceBounds(), surface->getSurfaceTransform());

            if (bounds.intersects(orientedBounds))
            {
                functor(surface, surfaceData.shader);
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
