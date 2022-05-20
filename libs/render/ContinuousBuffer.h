#pragma once

#include <cstdint>
#include <stack>
#include <limits>
#include <vector>
#include "igeometrystore.h"

namespace render
{

namespace detail
{

struct BufferTransaction
{
    IGeometryStore::Slot slot;
    std::size_t offset;
    std::size_t numChangedElements;
};

}

/**
 * Buffer object managing allocations within a continuous block of memory.
 * 
 * While the memory location itself might change when the buffer is growing,
 * the whole data is always stored in a single continuous memory block.
 * 
 * Use the allocate/deallocate methods to acquire or release a chunk of
 * a certain size. The chunk size is fixed and cannot be changed.
 */
template<typename ElementType>
class ContinuousBuffer
{
public:
    static constexpr std::size_t DefaultInitialSize = 65536;

    using Handle = std::uint32_t;

private:
    static constexpr std::size_t GrowthRate = 1; // 100% growth each time

    std::vector<ElementType> _buffer;

    struct SlotInfo
    {
        bool Occupied;      // whether this slot is free
        std::size_t Offset; // The index to the first element within the buffer
        std::size_t Size;   // Number of allocated elements
        std::size_t Used;   // Number of used elements

        SlotInfo() :
            Occupied(false),
            Offset(0),
            Size(0),
            Used(0)
        {}

        SlotInfo(std::size_t offset, std::size_t size, bool occupied) :
            Occupied(occupied),
            Offset(offset),
            Size(size),
            Used(0)
        {}
    };

    std::vector<SlotInfo> _slots;

    // A stack of slots that can be re-used instead
    std::stack<Handle> _emptySlots;

    // Last data size that was synced to the buffer object
    std::size_t _lastSyncedBufferSize;

    struct ModifiedMemoryChunk
    {
        Handle handle;
        std::size_t offset;
        std::size_t numElements;
    };

    // The slots that have been modified in between syncs
    std::vector<ModifiedMemoryChunk> _unsyncedModifications;

    std::size_t _allocatedElements;

public:
    ContinuousBuffer(std::size_t initialSize = DefaultInitialSize) :
        _lastSyncedBufferSize(0),
        _allocatedElements(0)
    {
        // Pre-allocate some memory, but don't go all the way down to zero
        _buffer.resize(initialSize == 0 ? 16 : initialSize);

        // The initial slot info which is going to be cut into pieces
        createSlotInfo(0, _buffer.size());
    }

    ContinuousBuffer(const ContinuousBuffer& other)
    {
        *this = other;
    }

    // Custom assignment operator
    ContinuousBuffer<ElementType>& operator=(const ContinuousBuffer<ElementType>& other)
    {
        _buffer.resize(other._buffer.size());
        memcpy(_buffer.data(), other._buffer.data(), other._buffer.size() * sizeof(ElementType));

        _slots.resize(other._slots.size());
        memcpy(_slots.data(), other._slots.data(), other._slots.size() * sizeof(SlotInfo));

        _emptySlots = other._emptySlots;
        _unsyncedModifications = other._unsyncedModifications;
        _allocatedElements = other._allocatedElements;

        return *this;
    }

    Handle allocate(std::size_t requiredSize)
    {
        auto handle = getNextFreeSlotForSize(requiredSize);

        _allocatedElements += requiredSize;

        return handle;
    }

    ElementType* getBufferStart()
    {
        return _buffer.data();
    }

    std::size_t getSize(Handle handle) const
    {
        return _slots[handle].Size;
    }

    std::size_t getNumUsedElements(Handle handle) const
    {
        return _slots[handle].Used;
    }

    std::size_t getOffset(Handle handle) const
    {
        return _slots[handle].Offset;
    }

    std::size_t getNumAllocatedElements() const
    {
        return _allocatedElements;
    }

    // The amount of memory used by this instance, in bytes
    std::size_t getBufferSizeInBytes() const
    {
        std::size_t total = 0;

        total += _buffer.capacity() * sizeof(ElementType);
        total += _slots.capacity() * sizeof(SlotInfo);
        total += _emptySlots.size() * sizeof(Handle);
        total += _unsyncedModifications.capacity() * sizeof(ModifiedMemoryChunk);
        total += sizeof(ContinuousBuffer<ElementType>);

        return total;
    }

    void setData(Handle handle, const std::vector<ElementType>& elements)
    {
        auto& slot = _slots[handle];

        auto numElements = elements.size();
        if (numElements > slot.Size)
        {
            throw std::logic_error("Cannot store more data than allocated in GeometryStore::Buffer::setData");
        }

        std::copy(elements.begin(), elements.end(), _buffer.begin() + slot.Offset);
        slot.Used = numElements;

        _unsyncedModifications.emplace_back(ModifiedMemoryChunk{ handle, 0, numElements });
    }

    void setSubData(Handle handle, std::size_t elementOffset, const std::vector<ElementType>& elements)
    {
        auto& slot = _slots[handle];

        auto numElements = elements.size();
        if (elementOffset + numElements > slot.Size)
        {
            throw std::logic_error("Cannot store more data than allocated in GeometryStore::Buffer::setSubData");
        }

        std::copy(elements.begin(), elements.end(), _buffer.begin() + slot.Offset + elementOffset);
        slot.Used = std::max(slot.Used, elementOffset + numElements);

        _unsyncedModifications.emplace_back(ModifiedMemoryChunk{ handle, elementOffset, numElements });
    }

    // Returns true if the size of this size actually changed
    bool resizeData(Handle handle, std::size_t elementCount)
    {
        auto& slot = _slots[handle];

        if (elementCount > slot.Size)
        {
            throw std::logic_error("Cannot resize to a large amount than allocated in GeometryStore::Buffer::resizeData");
        }

        if (slot.Used == elementCount) return false; // no size change

        slot.Used = elementCount;
        _unsyncedModifications.emplace_back(ModifiedMemoryChunk{ handle, 0, elementCount });
        return true;
    }

    void deallocate(Handle handle)
    {
        auto& releasedSlot = _slots[handle];
        releasedSlot.Occupied = false;
        releasedSlot.Used = 0;

        _allocatedElements -= releasedSlot.Size;

        // Check if the slot can merge with an adjacent one
        Handle slotIndexToMerge = std::numeric_limits<Handle>::max();
        if (findLeftFreeSlot(releasedSlot, slotIndexToMerge))
        {
            auto& slotToMerge = _slots[slotIndexToMerge];
            
            releasedSlot.Offset = slotToMerge.Offset;
            releasedSlot.Size += slotToMerge.Size;

            // The merged handle goes to recycling, block it against future use
            slotToMerge.Size = 0;
            slotToMerge.Used = 0;
            slotToMerge.Occupied = true;
            _emptySlots.push(slotIndexToMerge);
        }

        // Try to find an adjacent free slot to the right
        if (findRightFreeSlot(releasedSlot, slotIndexToMerge))
        {
            auto& slotToMerge = _slots[slotIndexToMerge];

            releasedSlot.Size += slotToMerge.Size;

            // The merged handle goes to recycling, block it against future use
            slotToMerge.Size = 0;
            slotToMerge.Used = 0;
            slotToMerge.Occupied = true;
            _emptySlots.push(slotIndexToMerge);
        }
    }

    void applyTransactions(const std::vector<detail::BufferTransaction>& transactions, const ContinuousBuffer<ElementType>& other,
        const std::function<std::uint32_t(IGeometryStore::Slot)>& getHandle)
    {
        // We might reach this point in single-buffer mode, trying to sync with ourselves
        // in which case we can take the shortcut to just mark the transactions that need to be GPU-synced
        if (&other == this)
        {
            for (const auto& transaction : transactions)
            {
                _unsyncedModifications.emplace_back(ModifiedMemoryChunk{
                    getHandle(transaction.slot), transaction.offset, transaction.numChangedElements });
            }

            return;
        }

        // Ensure the buffer is at least the same size
        auto otherSize = other._buffer.size();

        if (otherSize > _buffer.size())
        {
            _buffer.resize(otherSize);
        }

        for (const auto& transaction : transactions)
        {
            auto handle = getHandle(transaction.slot);
            auto& otherSlot = other._slots[handle];

            memcpy(_buffer.data() + otherSlot.Offset + transaction.offset,
                other._buffer.data() + otherSlot.Offset + transaction.offset,
                transaction.numChangedElements * sizeof(ElementType));

            // Remember this slot to be synced to the GPU
            _unsyncedModifications.emplace_back(ModifiedMemoryChunk{
                    handle, transaction.offset, transaction.numChangedElements });
        }

        // Replicate the slot allocation data
        _slots.resize(other._slots.size());
        memcpy(_slots.data(), other._slots.data(), other._slots.size() * sizeof(SlotInfo));

        _allocatedElements = other._allocatedElements;
        _emptySlots = other._emptySlots;
    }

    // Copies the updated memory to the given buffer object
    void syncModificationsToBufferObject(const IBufferObject::Ptr& buffer)
    {
        auto currentBufferSize = _buffer.size() * sizeof(ElementType);

        // On size change we upload everything
        if (_lastSyncedBufferSize != currentBufferSize)
        {
            // Resize the memory in the buffer object
            buffer->resize(currentBufferSize);
            _lastSyncedBufferSize = currentBufferSize;

            // Re-upload everything
            buffer->bind();
            buffer->setData(0, reinterpret_cast<unsigned char*>(_buffer.data()),
                _buffer.size() * sizeof(ElementType));
            buffer->unbind();
        }
        else
        {
            std::size_t minimumOffset = std::numeric_limits<std::size_t>::max();
            std::size_t maximumOffset = 0;

            std::size_t elementsToCopy = 0;

            // Size is the same, apply the updates to the GPU buffer
            // Determine the modified memory range
            for (auto modifiedChunk : _unsyncedModifications)
            {
                auto& slot = _slots[modifiedChunk.handle];

                minimumOffset = std::min(slot.Offset + modifiedChunk.offset, minimumOffset);
                maximumOffset = std::max(slot.Offset + modifiedChunk.offset + modifiedChunk.numElements, maximumOffset);
                elementsToCopy += modifiedChunk.numElements;
            }

            // Copy the data in one single operation or in multiple, depending on the effort
            if (elementsToCopy > 0)
            {
                buffer->bind();

                // Less than a couple of operations will be copied piece by piece
                if (_unsyncedModifications.size() < 100)
                {
                    for (auto modifiedChunk : _unsyncedModifications)
                    {
                        auto& slot = _slots[modifiedChunk.handle];

                        buffer->setData((slot.Offset + modifiedChunk.offset) * sizeof(ElementType),
                            reinterpret_cast<unsigned char*>(_buffer.data() + slot.Offset + modifiedChunk.offset),
                            modifiedChunk.numElements * sizeof(ElementType));
                    }
                }
                else // copy everything in between minimum and maximum in one operation
                {
                    buffer->setData(minimumOffset * sizeof(ElementType),
                        reinterpret_cast<unsigned char*>(_buffer.data() + minimumOffset),
                        (maximumOffset - minimumOffset) * sizeof(ElementType));
                }

                buffer->unbind();
            }
        }

        _unsyncedModifications.clear();
    }

private:
    bool findLeftFreeSlot(const SlotInfo& slotToTouch, Handle& found)
    {
        auto numSlots = _slots.size();

        for (Handle slotIndex = 0; slotIndex < numSlots; ++slotIndex)
        {
            const auto& candidate = _slots[slotIndex];

            if (candidate.Offset + candidate.Size == slotToTouch.Offset)
            {
                // The slot coordinates match, return true if this block is free
                found = slotIndex;
                return !candidate.Occupied;
            }
        }

        return false;
    }

    bool findRightFreeSlot(const SlotInfo& slotToTouch, Handle& found)
    {
        auto numSlots = _slots.size();
        auto offsetToMatch = slotToTouch.Offset + slotToTouch.Size;

        for (Handle slotIndex = 0; slotIndex < numSlots; ++slotIndex)
        {
            const auto& candidate = _slots[slotIndex];

            if (candidate.Offset == offsetToMatch)
            {
                // The slot coordinates match, return true if this block is free
                found = slotIndex;
                return !candidate.Occupied;
            }
        }

        return false;
    }

    Handle getNextFreeSlotForSize(std::size_t requiredSize)
    {
        auto numSlots = _slots.size();
        Handle rightmostFreeSlotIndex = static_cast<Handle>(numSlots);
        std::size_t rightmostFreeOffset = 0;
        std::size_t rightmostFreeSize = 0;
        Handle slotIndex = 0;

        for (slotIndex = 0; slotIndex < numSlots; ++slotIndex)
        {
            auto& slot = _slots[slotIndex];

            if (slot.Occupied) continue;

            // Keep track of the highest slot, we need that when re-allocating
            if (slot.Offset > rightmostFreeOffset)
            {
                rightmostFreeOffset = slot.Offset;
                rightmostFreeSize = slot.Size;
                rightmostFreeSlotIndex = slotIndex;
            }

            if (slot.Size < requiredSize) continue; // this slot is no use for us

            // Calculate the remaining size before assignment
            auto remainingSize = slot.Size - requiredSize;
            slot.Size = requiredSize;
            slot.Occupied = true;

            if (remainingSize > 0)
            {
                // Allocate a new free slot with the remaining space
                createSlotInfo(slot.Offset + requiredSize, remainingSize);
            }

            return slotIndex;
        }

        // No space wherever, we need to expand the buffer

        // Allocate more memory
        auto oldBufferSize = _buffer.size();
        auto additionalSize = std::max(oldBufferSize * GrowthRate, requiredSize);
        auto newSize = oldBufferSize + additionalSize;
        _buffer.resize(newSize);

        // Ensure that the rightmost free slot is at the end of the buffer, otherwise allocate a new one
        if (rightmostFreeSlotIndex == numSlots || rightmostFreeOffset + rightmostFreeSize != oldBufferSize)
        {
            // Create a free slot with 0 size at the end of the storage
            _slots.emplace_back(oldBufferSize, 0, false);

            // Adjust rightMostFreeSlotIndex to always point at this new slot
            rightmostFreeSlotIndex = static_cast<Handle>(numSlots);
        }

        // Use the right most slot for our requirement, then cut up the rest of the space
        auto& rightmostFreeSlot = _slots[rightmostFreeSlotIndex];

        assert(rightmostFreeSlot.Size < requiredSize); // otherwise we've run wrong above

        auto remainingSize = rightmostFreeSlot.Size + additionalSize - requiredSize;

        rightmostFreeSlot.Occupied = true;
        rightmostFreeSlot.Size = requiredSize;

        createSlotInfo(rightmostFreeSlot.Offset + rightmostFreeSlot.Size, remainingSize);

        return rightmostFreeSlotIndex;
    }

    SlotInfo& createSlotInfo(std::size_t offset, std::size_t size, bool occupied = false)
    {
        if (_emptySlots.empty())
        {
            return _slots.emplace_back(offset, size, occupied);
        }

        // Re-use an old slot
        auto& slot = _slots.at(_emptySlots.top());
        _emptySlots.pop();

        slot.Occupied = occupied;
        slot.Offset = offset;
        slot.Size = size;
        slot.Used = 0;

        return slot;
    }
};

}
