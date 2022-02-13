#pragma once

#include <cstdint>
#include <vector>
#include "math/AABB.h"
#include "render/ArbitraryMeshVertex.h"

namespace render
{

/**
 * Storage container for indexed vertex data.
 *
 * Client code will allocate fixed-size blocks of continuous
 * memory for vertex and index data.
 *
 * The Block handle will remain valid until relasing it,
 * though the underlying memory location is subject to change.
 *
 * Blocks cannot be resized after allocation.
 *
 * All the vertex data will is guaranteed to belong to the same continuous
 * large block of memory, making it suitable for openGL multi draw calls.
 */
class IGeometryStore
{
public:
    // Slot ID handed out to client code
    using Slot = std::uint64_t;

    virtual Slot allocateSlot(const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) = 0;

    virtual void updateData(Slot slot, const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) = 0;

    virtual void deallocateSlot(Slot slot) = 0;

    // The render parameters suitable for rendering surfaces using gl(Multi)DrawElements
    struct RenderParameters
    {
        ArbitraryMeshVertex* bufferStart;   // start of buffer (to pass to gl*Pointer)
        unsigned int* firstIndex;           // first index location of the given geometry
        std::size_t indexCount;             // index count of the given geometry
        std::size_t firstVertex;            // offset to the first vertex of this surface
    };

    // Returns the information necessary to render the given slot
    // Don't store these parameters on the client side, they will be only be valid 
    // for a certain amount of time, at the latest until allocateSlot or deallocateSlot are invoked.
    virtual RenderParameters getRenderParameters(Slot slot) = 0;

    // Returns the bounds of the geometry stored in the given slot
    virtual AABB getBounds(Slot slot) = 0;
};

}
