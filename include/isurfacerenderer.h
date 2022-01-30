#pragma once

#include "irenderableobject.h"
#include "igeometrystore.h"

#include <vector>
#include <limits>
#include <cstdint>
#include "render/ArbitraryMeshVertex.h"
#include "math/Matrix4.h"

namespace render
{

/**
 * Surface type consisting of triangles, oriented by a transformation matrix.
 * 
 * The ISurfaceRenderer will reacquire the transformation matrix each frame,
 * whereas the vertices and indices won't be requested every time.
 */
class IRenderableSurface :
    public IRenderableObject
{
public:
    using Ptr = std::shared_ptr<IRenderableSurface>;

    virtual ~IRenderableSurface() {}

    // Returns the vertex array of this surface
    virtual const std::vector<ArbitraryMeshVertex>& getVertices() = 0;

    // Returns the indices to render the triangle primitives
    virtual const std::vector<unsigned int>& getIndices() = 0;
};

/**
 * A surface renderer accepts a variable number of IRenderableSurfaces
 * each of which is oriented by its own transformation matrix.
 * 
 * Surfaces are rendered separately, the transformation matrix is requested 
 * each frame before drawing it.
 * The vertices and indices of the surface are buffered and won't be requested
 * every frame. Invoke the updateSurface() method to schedule an update.
 */
class ISurfaceRenderer
{
public:
    virtual ~ISurfaceRenderer() {}

    using Slot = std::uint64_t;
    static constexpr Slot InvalidSlot = std::numeric_limits<Slot>::max();

    // Allocate a slot to hold the given surface data, indexed to render triangles.
    // Returns the handle which can be used to update or deallocate the data later
    virtual Slot addSurface(IRenderableSurface& surface) = 0;

    // Releases a previously allocated slot. This invalidates the handle.
    virtual void removeSurface(Slot slot) = 0;

    // Schedules an update of the vertex data contained in the surface.
    virtual void updateSurface(Slot slot) = 0;

    // Submits the surface of a single slot to GL
    virtual void renderSurface(Slot slot) = 0;

    // Get the key to access the vertex data of this surface within the renderer's backend geometry store
    virtual IGeometryStore::Slot getSurfaceStorageLocation(ISurfaceRenderer::Slot slot) = 0;
};

}
