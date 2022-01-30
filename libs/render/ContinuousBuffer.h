#pragma once

#include <cstdint>
#include <limits>
#include <vector>

namespace render
{

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

        SlotInfo(bool occupied, std::size_t offset, std::size_t size) :
            Occupied(occupied),
            Offset(offset),
            Size(size)
        {}
    };

    std::vector<SlotInfo> _slots;

public:
    ContinuousBuffer(std::size_t initialSize = DefaultInitialSize)
    {
        // Pre-allocate some memory, but don't go all the way down to zero
        _buffer.resize(initialSize == 0 ? 16 : initialSize);

        // The initial slot info which is going to be cut into pieces
        _slots.emplace_back(SlotInfo{ false, 0, _buffer.size() });
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

    std::size_t getOffset(Handle handle) const
    {
        return _slots[handle].Offset;
    }

    void setData(Handle handle, const std::vector<ElementType>& elements)
    {
        const auto& slot = _slots[handle];

        if (elements.size() != slot.Size)
        {
            throw std::logic_error("Allocation size mismatch in GeometryStore::Buffer::setData");
        }

        std::copy(elements.begin(), elements.end(), _buffer.begin() + slot.Offset);
    }

    void deallocate(Handle handle)
    {
        auto& releasedSlot = _slots[handle];
        releasedSlot.Occupied = false;

        // Check if the slot can merge with an adjacent one
        if (handle > 0 && !_slots[handle - 1].Occupied)
        {
            // Merge the slot before the released one
            _slots[handle - 1].Size += releasedSlot.Size;

            // The released handle goes to waste, block it
            releasedSlot.Occupied = true;
            releasedSlot.Size = 0;
        }
        else if (handle < _slots.size() - 1 && !_slots[handle + 1].Occupied)
        {
            auto& nextSlot = _slots[handle + 1];

            releasedSlot.Size += nextSlot.Size;

            // The next slot will go to waste, never use that again
            nextSlot.Occupied = true;
            nextSlot.Size = 0;
        }
    }

private:
    Handle getNextFreeSlotForSize(std::size_t requiredSize)
    {
        auto numSlots = _slots.size();
        Handle rightmostFreeSlotIndex = 0;
        std::size_t rightmostFreeOffset = 0;

        for (Handle slotIndex = 0; slotIndex < numSlots; ++slotIndex)
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
                _slots.emplace_back(SlotInfo
                {
                    false,
                    slot.Offset + requiredSize,
                    remainingSize,
                });
            }

            return slotIndex;
        }

        // No space wherever, we need to expand the buffer
        auto additionalSize = std::max(_buffer.size() * GrowthRate, requiredSize);
        _buffer.resize(_buffer.size() + additionalSize);

        // Use the right most slot for our requirement, then cut up the rest of the space
        auto& rightmostFreeSlot = _slots[rightmostFreeSlotIndex];

        assert(rightmostFreeSlot.Size < requiredSize); // otherwise we've run wrong above

        auto remainingSize = rightmostFreeSlot.Size + additionalSize - requiredSize;

        rightmostFreeSlot.Occupied = true;
        rightmostFreeSlot.Size = requiredSize;

        _slots.emplace_back(SlotInfo
        {
            false,
            rightmostFreeSlot.Offset + rightmostFreeSlot.Size,
            remainingSize,
        });

        return rightmostFreeSlotIndex;
    }
};

}
