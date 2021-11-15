#pragma once

#include <vector>
#include <limits>
#include <cstdint>
#include "render/ArbitraryMeshVertex.h"

namespace render
{

enum class SurfaceIndexingType
{
    Triangles,
    Quads,
};

/**
 * A surface renderer accepts a variable number of indexed surfaces and arranges 
 * them into one or more continuous blocks of vertices for efficient rendering.
 *
 * The internal arrangement has the goal of reducing the amount of draw calls for
 * suraces sharing a single material. Allocating a surface slot yields a handle which
 * allows for later update or deallocation of the slot.
 */
class ISurfaceRenderer
{
public:
    virtual ~ISurfaceRenderer() {}

    using Slot = std::uint64_t;
    static constexpr Slot InvalidSlot = std::numeric_limits<Slot>::max();

    // Allocate a slot to hold the given surface data of the given size
    // Returns the handle which can be used to update or deallocate the data later
    // The indexType determines the primitive GLenum that is chosen to render this surface
    virtual Slot addSurface(SurfaceIndexingType indexType, 
        const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) = 0;

    // Releases a previously allocated slot. This invalidates the handle.
    virtual void removeSurface(Slot slot) = 0;

    // Sets the surface data
    virtual void updateSurface(Slot slot, const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) = 0;
};

}
