#pragma once

#include <map>
#include <sigc++/connection.h>
#include <sigc++/trackable.h>
#include <sigc++/functors/mem_fun.h>
#include "irender.h"
#include "irenderableobject.h"
#include "itextstream.h"

namespace entity
{

class RenderableObjectCollection :
    public sigc::trackable
{
private:
    AABB _collectionBounds;
    bool _collectionBoundsNeedUpdate;

    struct ObjectData
    {
        Shader* shader;
        sigc::connection boundsChangedConnection;
    };

    std::map<render::IRenderableObject::Ptr, ObjectData> _objects;

public:
    RenderableObjectCollection() :
        _collectionBoundsNeedUpdate(true)
    {}

    void addRenderable(const render::IRenderableObject::Ptr& object, Shader* shader)
    {
        sigc::connection subscription = object->signal_boundsChanged().connect(
            sigc::mem_fun(*this, &RenderableObjectCollection::onObjectBoundsChanged));
        
        if (!_objects.try_emplace(object, ObjectData{ shader, subscription }).second)
        {
            // We've already been subscribed to this one
            subscription.disconnect();
            rWarning() << "Renderable has already been attached to entity" << std::endl;
            return;
        }

        _collectionBoundsNeedUpdate = true;
    }

    void removeRenderable(const render::IRenderableObject::Ptr& object)
    {
        auto mapping = _objects.find(object);

        if (mapping != _objects.end())
        {
            mapping->second.boundsChangedConnection.disconnect();
            _objects.erase(mapping);
        }
        else
        {
            rWarning() << "Renderable has not been attached to entity" << std::endl;
        }

        _collectionBoundsNeedUpdate = true;
    }

    void foreachRenderable(const IRenderEntity::ObjectVisitFunction& functor)
    {
        ensureBoundsUpToDate();

        for (const auto& [object, objectData] : _objects)
        {
            functor(object, objectData.shader);
        }
    }

    void foreachRenderableTouchingBounds(const AABB& bounds,
        const IRenderEntity::ObjectVisitFunction& functor)
    {
        if (_objects.empty()) return;

        ensureBoundsUpToDate();

        // If the whole collection doesn't intersect, quit early
        if (!_collectionBounds.intersects(bounds)) return;
        
        for (const auto& [object, objectData] : _objects)
        {
            if (objectIntersectsBounds(bounds, *object))
            {
                functor(object, objectData.shader);
            }
        }
    }

private:
    bool objectIntersectsBounds(const AABB& bounds, render::IRenderableObject& object)
    {
        if (object.isOriented())
        {
            return bounds.intersects(AABB::createFromOrientedAABBSafe(
                object.getObjectBounds(), object.getObjectTransform()));
        }
        else
        {
            return bounds.intersects(object.getObjectBounds());
        }
    }

    void onObjectBoundsChanged()
    {
        _collectionBoundsNeedUpdate = true;
    }

    void ensureBoundsUpToDate()
    {
        if (!_collectionBoundsNeedUpdate) return;
        
        _collectionBoundsNeedUpdate = false;

        _collectionBounds = AABB();

        for (const auto& [object, _] : _objects)
        {
            _collectionBounds.includeAABB(AABB::createFromOrientedAABBSafe(
                object->getObjectBounds(), object->getObjectTransform()));
        }
    }
};

}
