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
    enum class Type
    {
        Allocate,
        Deallocate,
        Update,
    };

    IGeometryStore::Slot slot;
    Type type;
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

    // The slots that have been modified in between syncs
    std::vector<Handle> _unsyncedSlots;

public:
    ContinuousBuffer(std::size_t initialSize = DefaultInitialSize) :
        _lastSyncedBufferSize(0)
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
        _unsyncedSlots = other._unsyncedSlots;

        return *this;
    }

    Handle allocate(std::size_t requiredSize)
    {
        return getNextFreeSlotForSize(requiredSize);
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

        _unsyncedSlots.push_back(handle);
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

        _unsyncedSlots.push_back(handle);
    }

    void resizeData(Handle handle, std::size_t elementCount)
    {
        auto& slot = _slots[handle];

        if (elementCount > slot.Size)
        {
            throw std::logic_error("Cannot resize to a large amount than allocated in GeometryStore::Buffer::resizeData");
        }

        slot.Used = elementCount;

        _unsyncedSlots.push_back(handle);
    }

    void deallocate(Handle handle)
    {
        auto& releasedSlot = _slots[handle];
        releasedSlot.Occupied = false;
        releasedSlot.Used = 0;

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
        // Ensure the buffer is at least the same size
        auto otherSize = other._buffer.size();

        if (otherSize > _buffer.size())
        {
            _buffer.resize(otherSize);
        }

        for (const auto& transaction : transactions)
        {
            // Only the updated slots will actually have altered any data
            if (transaction.type == detail::BufferTransaction::Type::Update)
            {
                auto handle = getHandle(transaction.slot);
                auto& otherSlot = other._slots[handle];

                memcpy(_buffer.data() + otherSlot.Offset, other._buffer.data() + otherSlot.Offset, otherSlot.Size * sizeof(ElementType));

                // Remember this slot to be synced to the GPU
                _unsyncedSlots.push_back(handle);
            }
        }

        // Replicate the slot allocation data
        _slots.resize(other._slots.size());
        memcpy(_slots.data(), other._slots.data(), other._slots.size() * sizeof(SlotInfo));

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
            buffer->setData(0, reinterpret_cast<unsigned char*>(_buffer.data()),
                _buffer.size() * sizeof(ElementType));
        }
        else
        {
            std::size_t minimumOffset = std::numeric_limits<std::size_t>::max();
            std::size_t maximumOffset = 0;

            // Size is the same, apply the updates to the GPU buffer
            // Determine the modified memory range
            for (auto handle : _unsyncedSlots)
            {
                auto& slot = _slots[handle];

                minimumOffset = std::min(slot.Offset, minimumOffset);
                maximumOffset = std::max(slot.Offset + slot.Used, maximumOffset);
            }

            // Copy the data in one single operation
            if (!_unsyncedSlots.empty())
            {
                buffer->setData(minimumOffset * sizeof(ElementType),
                    reinterpret_cast<unsigned char*>(_buffer.data() + minimumOffset),
                    (maximumOffset - minimumOffset) * sizeof(ElementType));
            }
        }

        _unsyncedSlots.clear();
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
        Handle slotIndex = 0;

        for (slotIndex = 0; slotIndex < numSlots; ++slotIndex)
        {
            auto& slot = _slots[slotIndex];

            if (slot.Occupied) continue;

            // Keep track of the highest slot, we need that when re-allocating
            if (slot.Offset > rightmostFreeOffset)
            {
                rightmostFreeOffset = slot.Offset;
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

        // Check if we have any free slots, otherwise allocate a new one
        if (rightmostFreeSlotIndex == numSlots)
        {
            // Create a free slot with 0 size,
            // rightMostFreeSlotIndex is now within the valid range
            _slots.emplace_back(_buffer.size(), 0, false);
        }

        // Allocate more memory
        auto additionalSize = std::max(_buffer.size() * GrowthRate, requiredSize);
        auto newSize = _buffer.size() + additionalSize;
        _buffer.resize(newSize);

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
