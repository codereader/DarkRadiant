#pragma once

#include <vector>
#include <limits>
#include <cstdint>
#include "igeometrystore.h"
#include "render/RenderVertex.h"
#include "math/AABB.h"

namespace render
{

enum class GeometryType
{
    Triangles,
    Quads,
    Lines,
    Points,
};

/**
 * A geometry renderer accepts a variable number of indexed vertices and internally
 * arranges them into one or more continuous blocks for efficient rendering.
 *
 * The internal arrangement has the goal of reducing the amount of draw calls for
 * primitives sharing a single material. Allocating a geometry slot yields a handle which
 * allows for later update or deallocation of the slot.
 */
class IGeometryRenderer
{
public:
    virtual ~IGeometryRenderer() {}

    using Slot = std::uint64_t;
    static constexpr Slot InvalidSlot = std::numeric_limits<Slot>::max();

    // Allocate a slot to hold the given indexed vertex data.
    // Returns the handle which can be used to update or deallocate the data later
    // The indexType determines the primitive GLenum that is chosen to render this surface
    virtual Slot addGeometry(GeometryType indexType,
        const std::vector<RenderVertex>& vertices,
        const std::vector<unsigned int>& indices) = 0;

    // Releases a previously allocated slot. This invalidates the handle.
    virtual void removeGeometry(Slot slot) = 0;

    // Updates the vertex data. The size of the vertex and index array must be the same
    // as the one passed to addGeometry. To change the size the data needs to be removed and re-added.
    virtual void updateGeometry(Slot slot, const std::vector<RenderVertex>& vertices,
        const std::vector<unsigned int>& indices) = 0;

    // Submits the geometry of a single slot to GL
    virtual void renderGeometry(Slot slot) = 0;

    // Returns the bounding box of the geometry stored in the given slot
    virtual AABB getGeometryBounds(Slot slot) = 0;

    // Returns the storage handle to enable the backend renderer to get hold of the indexed vertex data
    virtual IGeometryStore::Slot getGeometryStorageLocation(Slot slot) = 0;
};

}
