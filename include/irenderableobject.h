#pragma once

#include <memory>
#include <sigc++/signal.h>
#include "igeometrystore.h"
#include "math/AABB.h"
#include "math/Matrix4.h"

namespace render
{

class IRenderableObject
{
public:
    using Ptr = std::shared_ptr<IRenderableObject>;

    virtual ~IRenderableObject() {}

    // Returns true if this object is not empty and should be rendered
    virtual bool isVisible() = 0;

    // The model view matrix used to render this object
    virtual const Matrix4& getObjectTransform() = 0;

    // The object bounds in local coordinates
    virtual const AABB& getObjectBounds() = 0;

    // Emitted when the object bounds have changed,
    // because it has been either moved or resized.
    virtual sigc::signal<void>& signal_boundsChanged() = 0;

    // Returns the key to access the vertex data in the renderer's geometry store
    virtual IGeometryStore::Slot getStorageLocation() = 0;
};

}
