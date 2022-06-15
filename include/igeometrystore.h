#pragma once

#include <cstdint>
#include <vector>
#include "math/AABB.h"
#include "render/RenderVertex.h"

namespace render
{

// A sync object guarding a single frame buffer (glFenceSync)
class ISyncObject
{
public:
    using Ptr = std::shared_ptr<ISyncObject>;

    virtual ~ISyncObject() {}

    // Blocks until the guarded buffer is no longer in use
    virtual void wait() = 0;
};

// Creates sync objects to safely switch between frame buffers
// An implementation of this interface may be required to construct an IGeometryStore
class ISyncObjectProvider
{
public:
    virtual ~ISyncObjectProvider() {}

    // Create a sync object (fence)
    virtual ISyncObject::Ptr createSyncObject() = 0;
};

// Represents a buffer object as provided by the openGL implementation
// Operates on byte (unsigned char) data types.
class IBufferObject
{
public:
    virtual ~IBufferObject() {}

    enum class Type
    {
        Vertex, // vertex buffer
        Index   // index buffer
    };

    using Ptr = std::shared_ptr<IBufferObject>;

    // Binds this object (glBindBuffer)
    virtual void bind() = 0;

    // Release the binding, aka glBindBuffer(0)
    virtual void unbind() = 0;

    // Uploads the given data to the buffer, starting at the given offset
    virtual void setData(std::size_t offset, const unsigned char* firstElement, std::size_t numBytes) = 0;

    // Re-allocates the memory of this buffer, does not transfer the data 
    // from the old internal buffer to the new one.
    virtual void resize(std::size_t newSize) = 0;
};

// Factory class creating new buffer objects
class IBufferObjectProvider
{
public:
    virtual ~IBufferObjectProvider() {}

    // Creates a new, empty buffer object of 0 size. Has to be resized before use.
    virtual IBufferObject::Ptr createBufferObject(IBufferObject::Type type) = 0;
};

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
    virtual ~IGeometryStore() {}

    // Slot ID handed out to client code
    using Slot = std::uint64_t;

    /**
     * Allocate memory blocks, one for vertices and one for indices, of the given size.
     * The block can be populated using updateData(), where it's possible to
     * fill the entire block or just a portion of it.
     * Returns a handle as reference to the block for use in later calls.
     * The allocated block cannot be resized later.
     */
    virtual Slot allocateSlot(std::size_t numVertices, std::size_t numIndices) = 0;

    /**
     * Allocate memory to store an alternative set of indices referencing
     * an existing set of vertices. When rendering this re-mapped geometry, it will
     * re-use the vertices of that other slot with the indices defined in this slot.
     *
     * With the returned handle, user code can invoke the update[Sub]Data() methods
     * to upload the indices, but the passed vertex set has to be empty.
     *
     * Use the regular deallocate() method to release this slot.
     *
     * The index remap slot is depending on the one containing the vertex data, if the latter
     * is removed, this slot becomes invalid and behaviour is undefined.
     */
    virtual Slot allocateIndexSlot(Slot slotContainingVertexData, std::size_t numIndices) = 0;

    /**
     * Load vertex and index data into the specified block. The given vertex and
     * index arrays must not be larger than what has been allocated earlier, 
     * but they're allowed to be smaller.
     */
    virtual void updateData(Slot slot, const std::vector<RenderVertex>& vertices,
        const std::vector<unsigned int>& indices) = 0;

    /**
     * Updates the data of an index slot. Equivalent to calling updateData() with
     * an empty set of vertices.
     */
    virtual void updateIndexData(Slot slot, const std::vector<unsigned int>& indices)
    {
        updateData(slot, {}, indices);
    }

    /**
     * Load a chunk of vertex and index data into the specified range, starting
     * from vertexOffset/indexOffset respectively. The affected range must not be out of bounds
     * of the allocated slot.
     */
    virtual void updateSubData(Slot slot, std::size_t vertexOffset, const std::vector<RenderVertex>& vertices,
        std::size_t indexOffset, const std::vector<unsigned int>& indices) = 0;

    /**
     * Updates a portion of index data in an index slot. Equivalent to calling updateSubData() with
     * an empty set of vertices.
     */
    virtual void updateIndexSubData(Slot slot, std::size_t indexOffset, const std::vector<unsigned int>& indices)
    {
        updateSubData(slot, 0, {}, indexOffset, indices);
    }

    /**
     * Called in case the stored data in the given slot should just be cut off at the end.
     */
    virtual void resizeData(Slot slot, std::size_t vertexSize, std::size_t indexSize) = 0;

    /**
     * Resize the index data in an index slot. Equivalent to calling resizeData() with vertexSize == 0.
     */
    virtual void resizeIndexData(Slot slot, std::size_t indexSize)
    {
        resizeData(slot, 0, indexSize);
    }

    /**
     * Releases the memory allocated by the given slot.
     * The Slot ID is invalidated by this operation and should no longer be used.
     */
    virtual void deallocateSlot(Slot slot) = 0;

    // The render parameters suitable for rendering surfaces using gl(Multi)DrawElements
    struct RenderParameters
    {
        RenderVertex* bufferStart;        // start of buffer (to pass to gl*Pointer, usually nullptr)
        RenderVertex* clientBufferStart;  // start of buffer in client memory
        unsigned int* firstIndex;         // first index location of the given geometry (to pass to glDraw*)
        unsigned int* clientFirstIndex;   // first index location of the given geometry in client memory
        std::size_t indexCount;           // index count of the given geometry
        std::size_t firstVertex;          // offset to the first vertex of this surface
    };

    // Returns the information necessary to render the given slot
    // Don't store these parameters on the client side, they will be only be valid 
    // for a certain amount of time, at the latest until allocateSlot or deallocateSlot are invoked.
    virtual RenderParameters getRenderParameters(Slot slot) = 0;

    /**
     * Returns the bounds of the geometry stored in the given slot.
     * Note that this will only take those vertices into account 
     * that are actually referenced by any index in the slot.
     */ 
    virtual AABB getBounds(Slot slot) = 0;

    // Return the buffer objects of the current frame
    virtual std::pair<IBufferObject::Ptr, IBufferObject::Ptr> getBufferObjects() = 0;

    // Synchronises the data in the currently active framebuffer to the attached IBufferObjects
    virtual void syncToBufferObjects() = 0;
};

}
