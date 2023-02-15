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

    /// List of vertices
    using Vertices = std::vector<RenderVertex>;

    /// List of indices
    using Indices = std::vector<unsigned int>;

    /**
     * @brief Allocate a slot to hold the given indexed vertex data.
     *
     * Added geometry is active by default.
     *
     * @param primType Determines the primitive GLenum that is chosen to render this surface
     * @param vertices Vector of vertex data
     * @param indices Vector of index data
     * @return Slot which can be used to update or deallocate the data later
     */
    virtual Slot
    addGeometry(GeometryType primType, const Vertices& vertices, const Indices& indices) = 0;

    // Re-activates a previously deactivated geometry slot.
    virtual void activateGeometry(Slot slot) = 0;

    // Suspends the geometry in the given slot - it won't be rendered anymore.
    // Doesn't remove any of the vertex or index data (use removeGeometry() for that).
    // The slot handle will remain valid.
    // Inactive geometry can automatically be re-activated by updateGeometry(),
    // or an explicit activateGeometry() call.
    virtual void deactivateGeometry(Slot slot) = 0;

    // Releases a previously allocated slot. This invalidates the handle.
    virtual void removeGeometry(Slot slot) = 0;

    // Updates the vertex data. The size of the vertex and index array must be the same
    // as the one passed to addGeometry. To change the size the data needs to be removed and re-added.
    virtual void updateGeometry(Slot slot, const Vertices& vertices, const Indices& indices) = 0;

    // Submits all active geometry slots to GL
    virtual void renderAllVisibleGeometry() = 0;

    // Submits the geometry of a single slot to GL, regardless of its active state
    virtual void renderGeometry(Slot slot) = 0;

    // Returns the bounding box of the geometry stored in the given slot
    virtual AABB getGeometryBounds(Slot slot) const = 0;

    // Returns the storage handle to enable the backend renderer to get hold of the indexed vertex data
    virtual IGeometryStore::Slot getGeometryStorageLocation(Slot slot) = 0;
};

}
