#pragma once

#include <vector>

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

        auto currentSize = _vertices.size();
        auto position = currentSize / _size;
        _vertices.reserve(currentSize + _size); // reserve() never shrinks

        std::copy(winding.begin(), winding.end(), std::back_inserter(_vertices));

        // Allocate and calculate indices
        _indices.reserve(_indices.size() + _numIndicesPerWinding);

        for (unsigned int n = static_cast<unsigned int>(_size) - 1; n - 1 > 0; --n)
        {
            _indices.push_back(0);
            _indices.push_back(n - 1);
            _indices.push_back(n);
        }

        return position;
    }
};

}
