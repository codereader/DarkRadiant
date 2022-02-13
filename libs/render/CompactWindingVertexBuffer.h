#pragma once

#include <vector>
#include <stdexcept>

namespace render
{

// Winding index provider. Generates render indices for a single winding suitable for rendering triangles.
// Indices are generated in CCW order.
class WindingIndexer_Triangles
{
public:
    constexpr static std::size_t GetNumberOfIndicesPerWinding(const std::size_t windingSize)
    {
        return 3 * (windingSize - 2);
    }

    // Generate indices for a single winding of the given size, insert it in the target container using the given output iterator
    // each index is shifted by the given offset
    static void GenerateAndAssignIndices(std::back_insert_iterator<std::vector<unsigned int>> outputIt, 
        std::size_t windingSize, const unsigned int offset)
    {
        for (auto n = static_cast<unsigned int>(windingSize) - 1; n - 1 > 0; --n)
        {
            outputIt = offset + 0;
            outputIt = offset + n - 1;
            outputIt = offset + n;
        }
    }
};

// Winding index provider. Generates render indices for a single winding suitable for rendering lines.
class WindingIndexer_Lines
{
public:
    constexpr static std::size_t GetNumberOfIndicesPerWinding(const std::size_t windingSize)
    {
        return windingSize * 2; // 2 indices per winding
    }

    // Generate indices for a single winding of the given size, insert it in the target container using the given output iterator
    // each index is shifted by the given offset
    static void GenerateAndAssignIndices(std::back_insert_iterator<std::vector<unsigned int>> outputIt,
        std::size_t windingSize, const unsigned int offset)
    {
        for (unsigned int n = 0; n < windingSize - 1; ++n)
        {
            outputIt = offset + n + 0;
            outputIt = offset + n + 1;
        }

        outputIt = offset + static_cast<unsigned int>(windingSize) - 1;
        outputIt = offset; // the last index points at the first vertex
    }
};

// Winding index provider. Generates render indices for a single winding suitable for drawing a single polygon.
class WindingIndexer_Polygon
{
public:
    constexpr static std::size_t GetNumberOfIndicesPerWinding(const std::size_t windingSize)
    {
        return windingSize;
    }

    // Generate indices for a single winding of the given size, insert it in the target container using the given output iterator
    // each index is shifted by the given offset
    static void GenerateAndAssignIndices(std::back_insert_iterator<std::vector<unsigned int>> outputIt,
        std::size_t windingSize, const unsigned int offset)
    {
        for (unsigned int n = 0; n < windingSize; ++n)
        {
            outputIt = offset + n;
        }
    }
};

template<typename VertexT, class WindingIndexerT = WindingIndexer_Triangles>
class CompactWindingVertexBuffer
{
private:
    std::size_t _size;

    std::vector<VertexT> _vertices;

    // The indices suitable for rendering triangles
    std::vector<unsigned int> _indices;

public:
    using Slot = std::uint32_t;

    explicit CompactWindingVertexBuffer(std::size_t size) :
        _size(size)
    {}

    CompactWindingVertexBuffer(const CompactWindingVertexBuffer& other) = delete;
    CompactWindingVertexBuffer& operator=(const CompactWindingVertexBuffer& other) = delete;

    // Move ctor
    CompactWindingVertexBuffer(CompactWindingVertexBuffer&& other) noexcept :
        _size(other._size),
        _vertices(std::move(other._vertices)),
        _indices(std::move(other._indices))
    {}

    std::size_t getWindingSize() const
    {
        return _size;
    }

    std::size_t getNumIndicesPerWinding() const
    {
        return WindingIndexerT::GetNumberOfIndicesPerWinding(_size);
    }

    const std::vector<VertexT>& getVertices() const
    {
        return _vertices;
    }

    const std::vector<unsigned int>& getIndices() const
    {
        return _indices;
    }

    // Appends the given winding data to the end of the buffer, returns the position in the array
    Slot pushWinding(const std::vector<VertexT>& winding)
    {
        assert(winding.size() == _size);

        const auto currentSize = _vertices.size();
        
        std::copy(winding.begin(), winding.end(), std::back_inserter(_vertices));

        WindingIndexerT::GenerateAndAssignIndices(std::back_inserter(_indices), _size, static_cast<unsigned int>(currentSize));

        auto position = currentSize / _size;
        return static_cast<Slot>(position);
    }

    // Replaces the winding in the given slot with the given data
    void replaceWinding(Slot slot, const std::vector<VertexT>& winding)
    {
        assert(winding.size() == _size);

        // Copy the incoming data to the target slot
        std::copy(winding.begin(), winding.end(), _vertices.begin() + (slot * _size));

        // Indices remain unchanged
    }

    // Removes the winding from the given slot. All slots greater than the given one
    // will be shifted towards the left, their values are shifted by -1
    // Invalid slot indices will result in a std::logic_error
    void removeWinding(Slot slot)
    {
        const auto currentSize = _vertices.size();

        if (slot >= currentSize / _size) throw std::logic_error("Slot index out of bounds");

        // Remove _size elements at the given position
        auto firstVertexToRemove = _vertices.begin() + (slot * _size);
        _vertices.erase(firstVertexToRemove, firstVertexToRemove + _size);

        // Since all the windings have the same structure, the index array will always look the same
        // after shifting the index values of the remaining windings. 
        // So just cut off one winding from the end of the index array
        _indices.resize(_indices.size() - getNumIndicesPerWinding());
    }

    // Removes multiple slots from this buffer in one sweep. The given array of slots must be sorted in ascending order
    void removeWindings(const std::vector<Slot>& slotsToRemove)
    {
        if (slotsToRemove.empty()) return;

        auto highestPossibleSlotNumber = static_cast<Slot>(_vertices.size() / _size);

        auto s = slotsToRemove.begin();
        auto gapStart = *s; // points at the first position that can be overwritten

        while (s != slotsToRemove.end())
        {
            auto slotToRemove = *s;

            if (slotToRemove >= highestPossibleSlotNumber) throw std::logic_error("Slot index out of bounds");

            // Move forward until we hit the next unremoved slot
            auto firstSlotToKeep = slotToRemove + 1;
            ++s;

            while (s != slotsToRemove.end() && *s == firstSlotToKeep)
            {
                ++firstSlotToKeep;
                ++s;
            }

            auto lastSlotToKeep = s == slotsToRemove.end() ? highestPossibleSlotNumber - 1 : *s - 1;
            auto numSlotsToMove = lastSlotToKeep + 1 - firstSlotToKeep;

            if (numSlotsToMove == 0) break;

            // We move all vertices to the gap
            auto target = _vertices.begin() + (gapStart * _size);

            auto sourceStart = _vertices.begin() + (firstSlotToKeep * _size);
            auto sourceEnd = sourceStart + (numSlotsToMove * _size);

            std::move(sourceStart, sourceEnd, target);

            gapStart += numSlotsToMove;
        }

        // Cut off the now unused range at the end
        _vertices.resize(_vertices.size() - slotsToRemove.size() * _size);

        // Since all the windings have the same structure, the index array will always look the same
        // after shifting the index values of the remaining windings. 
        // So just cut off one winding from the end of the index array
        _indices.resize(_indices.size() - slotsToRemove.size() * getNumIndicesPerWinding());
    }
};

}
