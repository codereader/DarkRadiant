#pragma once

#include <vector>
#include <stdexcept>

namespace render
{

template<typename VertexT>
class CompactWindingVertexBuffer
{
private:
    std::size_t _size;
    std::size_t _numIndicesPerWinding;

    std::vector<VertexT> _vertices;

    // The indices suitable for rendering triangles
    std::vector<unsigned int> _indices;

public:
    CompactWindingVertexBuffer(std::size_t size) :
        _size(size),
        _numIndicesPerWinding(3 * (_size - 2))
    {}

    CompactWindingVertexBuffer(const CompactWindingVertexBuffer& other) = delete;
    CompactWindingVertexBuffer& operator=(const CompactWindingVertexBuffer& other) = delete;

    std::size_t getWindingSize() const
    {
        return _size;
    }

    std::size_t getNumIndicesPerWinding() const
    {
        return _numIndicesPerWinding;
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
    std::size_t pushWinding(const std::vector<VertexT>& winding)
    {
        assert(winding.size() == _size);

        const auto currentSize = _vertices.size();
        auto position = currentSize / _size;
        _vertices.reserve(currentSize + _size); // reserve() never shrinks

        std::copy(winding.begin(), winding.end(), std::back_inserter(_vertices));

        // Allocate and calculate indices
        _indices.reserve(_indices.size() + _numIndicesPerWinding);

        for (unsigned int n = static_cast<unsigned int>(_size) - 1; n - 1 > 0; --n)
        {
            _indices.push_back(static_cast<unsigned int>(currentSize) + 0);
            _indices.push_back(static_cast<unsigned int>(currentSize) + n - 1);
            _indices.push_back(static_cast<unsigned int>(currentSize) + n);
        }

        return position;
    }

    // Removes the winding from the given slot. All slots greater than the given one
    // will be shifted towards the left, their values are shifted by -1
    // Invalid slot indices will result in a std::logic_error
    void removeWinding(std::size_t slot)
    {
        const auto currentSize = _vertices.size();

        if (slot >= currentSize / _size) throw std::logic_error("Slot index out of bounds");

        // Remove _size elements at the given position
        auto firstVertexToRemove = _vertices.begin() + (slot * _size);
        _vertices.erase(firstVertexToRemove, firstVertexToRemove + _size);

        // Since all the windings have the same structure, the index array will always look the same
        // after shifting the index values of the remaining windings. So just cut off the last one
#if 0
        // Shift all indices after this slot towards the left, applying the -size offset
        auto firstIndexToAdjust = (slot + 1) * _numIndicesPerWinding;
        auto lastIndexToAdjust = _indices.size() - 1;
        auto indexToReassign = slot * _numIndicesPerWinding;

        for (auto i = firstIndexToAdjust; i < lastIndexToAdjust; ++i, ++indexToReassign)
        {
            _indices[indexToReassign] = _indices[i] - static_cast<unsigned int>(_size);
        }
#endif
        // Cut off one winding from the end of the index array
        _indices.resize(_indices.size() - _numIndicesPerWinding);
    }
};

}
