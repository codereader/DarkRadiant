#pragma once

#include <cstdint>
#include <vector>
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

    virtual void deallocateSlot(Slot slot) = 0;
};

}
