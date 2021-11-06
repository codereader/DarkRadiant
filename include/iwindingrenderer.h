#pragma once

#include <vector>
#include <limits>
#include <cstdint>
#include "render/ArbitraryMeshVertex.h"

namespace render
{

/**
 * A winding renderer accepts a variable number of windings and arranges them into 
 * one or more continuous blocks of vertices for efficient rendering.
 * 
 * The internal arrangement has the goal of reducing the amount of draw calls for 
 * winding sharing a single material. Allocating a winding slot yields a handle which
 * allows for later update or deallocation of the slot.
 * 
 * Only the vertex data (XYZ, UV, Normals, Colour) needs to be submitted,
 * the render indices of each winding slot are handled internally.
 */
class IWindingRenderer
{
public:
    virtual ~IWindingRenderer() {}

    using Slot = std::uint64_t;
    static constexpr Slot InvalidSlot = std::numeric_limits<std::size_t>::max();

    // Allocate a slot to hold the vertex data of a winding of the given size
    // Returns the handle which can be used to update or deallocate the data later
    virtual Slot addWinding(const std::vector<ArbitraryMeshVertex>& vertices) = 0;

    // Releases a previously allocated winding slot. This invalidates the handle.
    virtual void removeWinding(Slot slot) = 0;

    // Sets the winding data
    virtual void updateWinding(Slot slot, const std::vector<ArbitraryMeshVertex>& vertices) = 0;
};

}
