#pragma once

#include "igeometryrenderer.h"
#include "igeometrystore.h"

namespace render
{

class GeometryRenderer :
    public IGeometryRenderer
{
private:
    class VertexBuffer
    {
    private:
        IGeometryStore& _store;
        GLenum _mode;

        std::vector<ArbitraryMeshVertex> _vertices;
        std::vector<unsigned int> _indices;
        
        // The input arrays used for glMultiDrawElementsBaseVertex
        // when drawing the entire buffer at once (non-lit mode)
        std::vector<GLsizei> _sizes;
        std::vector<void*> _firstIndices;
        std::vector<GLint> _firstVertices;

    public:
        VertexBuffer(IGeometryStore& store, GLenum mode) :
            _store(store),
            _mode(mode)
        {}

        bool empty() const
        {
            return _indices.empty();
        }

        void render(bool renderBump)
        {
            if (_indices.empty()) return;

            glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().vertex);
            glColorPointer(4, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().colour);

            if (renderBump)
            {
                glVertexAttribPointer(ATTR_NORMAL, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &_vertices.front().normal);
                glVertexAttribPointer(ATTR_TEXCOORD, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &_vertices.front().texcoord);
                glVertexAttribPointer(ATTR_TANGENT, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &_vertices.front().tangent);
                glVertexAttribPointer(ATTR_BITANGENT, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &_vertices.front().bitangent);
            }
            else
            {
                glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().texcoord);
                glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().normal);
            }

            glMultiDrawElementsBaseVertex(_mode, _sizes.data(), GL_UNSIGNED_INT,
                &_firstIndices.front(), static_cast<GLsizei>(_sizes.size()), &_firstVertices.front());
        }

        void renderSurface(std::size_t surfaceIndex) const
        {
            glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().vertex);
            glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().texcoord);
            glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().normal);

            glDrawElementsBaseVertex(_mode, _sizes[surfaceIndex], GL_UNSIGNED_INT,
                _firstIndices[surfaceIndex], _firstVertices[surfaceIndex]);
        }

        // Returns the surface index within this buffer
        std::size_t addSurface(const std::vector<ArbitraryMeshVertex>& vertices,
            const std::vector<unsigned int>& indices)
        {
            _sizes.push_back(static_cast<GLsizei>(indices.size()));
            _firstVertices.push_back(static_cast<GLint>(_vertices.size()));

            std::copy(vertices.begin(), vertices.end(), std::back_inserter(_vertices));
            std::copy(indices.begin(), indices.end(), std::back_inserter(_indices));

            // Rebuild the index array to point at the (most likely) new memory locations
            auto numSurfaces = _sizes.size();
            _firstIndices.resize(numSurfaces);

            auto currentLocation = const_cast<unsigned int*>(&_indices.front());

            for (auto i = 0; i < numSurfaces; ++i)
            {
                _firstIndices[i] = currentLocation;
                currentLocation += _sizes[i];
            }

            return numSurfaces - 1;
        }

        void updateSurface(std::size_t surfaceIndex,
            const std::vector<ArbitraryMeshVertex>& vertices,
            const std::vector<unsigned int>& indices)
        {
            auto firstVertex = _firstVertices.at(surfaceIndex);

            // Calculate the distance within the index buffer from the pointer difference
            auto* surfaceStartIndex = static_cast<unsigned int*>(_firstIndices.at(surfaceIndex));
            auto firstIndex = surfaceStartIndex - &_indices.front();

            // Copy the data to the correct slot in the array
            std::copy(vertices.begin(), vertices.end(), _vertices.begin() + firstVertex);
            std::copy(indices.begin(), indices.end(), _indices.begin() + firstIndex);
        }

        // Cuts out the vertices and indices, adjusts all metadata
        void removeSurface(std::size_t surfaceIndex, std::size_t numVertices)
        {
            auto firstVertex = _firstVertices.at(surfaceIndex);

            // Cut out the vertices
            auto firstVertexToRemove = _vertices.begin() + firstVertex;
            _vertices.erase(firstVertexToRemove, firstVertexToRemove + numVertices);

            // Cut out the indices
            auto* surfaceStartIndex = static_cast<unsigned int*>(_firstIndices.at(surfaceIndex));
            auto firstIndex = surfaceStartIndex - &_indices.front();

            auto firstIndexToRemove = _indices.begin() + firstIndex;
            _indices.erase(firstIndexToRemove, firstIndexToRemove + _sizes.at(surfaceIndex));

            // Adjust the metadata needed for glMultiDrawElementsBaseVertex
            
            // Shift the vertex starts by the amount of removed vertices
            auto firstVertexToAdjust = _firstVertices.begin() + surfaceIndex;

            while (++firstVertexToAdjust != _firstVertices.end())
            {
                *firstVertexToAdjust -= static_cast<GLint>(numVertices);
            }

            _firstVertices.erase(_firstVertices.begin() + surfaceIndex);

            // Shift the index start pointers, assuming that the buffer has not been reallocated
            auto firstIndexToAdjust = _firstIndices.begin() + surfaceIndex;
            auto numIndicesRemoved = _sizes.at(surfaceIndex);

            while (++firstIndexToAdjust != _firstIndices.end())
            {
                *firstIndexToAdjust = static_cast<unsigned int*>(*firstIndexToAdjust) - numIndicesRemoved;
            }

            _firstIndices.erase(_firstIndices.begin() + surfaceIndex);

            // Index size is reduced by one
            _sizes.erase(_sizes.begin() + surfaceIndex);
        }
    };

    std::vector<VertexBuffer> _buffers;

    static constexpr std::size_t InvalidSurfaceIndex = std::numeric_limits<std::size_t>::max();

    // Internal information about where the chunk of indexed vertex data is located:
    // Which buffer they're in, and the data offset and count within the buffer.
    // This is enough information to access, replace or remove the data at a later point.
    struct SlotInfo
    {
        std::uint8_t bufferIndex;
        std::size_t surfaceIndex;
        std::size_t numVertices;
        std::size_t numIndices;
        IGeometryStore::Slot storageHandle;
    };
    std::vector<SlotInfo> _slots;

    static constexpr std::size_t InvalidSlotMapping = std::numeric_limits<std::size_t>::max();
    std::size_t _freeSlotMappingHint;

public:
    GeometryRenderer(IGeometryStore& store) :
        _freeSlotMappingHint(InvalidSlotMapping)
    {
        _buffers.emplace_back(store, GL_TRIANGLES);
        _buffers.emplace_back(store, GL_QUADS);
        _buffers.emplace_back(store, GL_LINES);
        _buffers.emplace_back(store, GL_POINTS);
    }

    bool empty() const
    {
        for (const auto& buffer : _buffers)
        {
            if (!buffer.empty()) return false;
        }

        return true;
    }

    Slot addGeometry(GeometryType indexType, const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        auto bufferIndex = GetBufferIndexForIndexType(indexType);
        auto& buffer = getBufferByIndex(bufferIndex);

        // Allocate a slot
        auto newSlotIndex = getNextFreeSlotMapping();
        auto& slot = _slots.at(newSlotIndex);

        slot.surfaceIndex = buffer.addSurface(vertices, indices);

        slot.bufferIndex = bufferIndex;
        slot.numVertices = vertices.size();
        slot.numIndices = indices.size();

        return newSlotIndex;
    }

    void removeGeometry(Slot slot) override
    {
        auto& slotInfo = _slots.at(slot);
        auto& bucket = getBufferByIndex(slotInfo.bufferIndex);

        bucket.removeSurface(slotInfo.surfaceIndex, slotInfo.numVertices);

        // Adjust all offsets in other slots pointing to the same bucket
        for (auto& slot : _slots)
        {
            if (slot.bufferIndex == slotInfo.bufferIndex && 
                slot.surfaceIndex > slotInfo.surfaceIndex &&
                slot.surfaceIndex != InvalidSurfaceIndex)
            {
                --slot.surfaceIndex;
            }
        }

        // Invalidate the slot
        slotInfo.numVertices = 0;
        slotInfo.surfaceIndex = InvalidSurfaceIndex;
        slotInfo.numIndices = 0;
        
        if (slot < _freeSlotMappingHint)
        {
            _freeSlotMappingHint = slot;
        }
    }

    void updateGeometry(Slot slot, const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        auto& slotInfo = _slots.at(slot);

        if (slotInfo.numVertices != vertices.size() ||
            slotInfo.numIndices != indices.size())
        {
            throw std::logic_error("updateGeometry: Data size mismatch");
        }

        auto& bucket = getBufferByIndex(slotInfo.bufferIndex);

        bucket.updateSurface(slotInfo.surfaceIndex, vertices, indices);
    }

    void render(const RenderInfo& info)
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        glFrontFace(GL_CW);

        for (auto& buffer : _buffers)
        {
            buffer.render(info.checkFlag(RENDER_BUMP));
        }

        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    void renderGeometry(Slot slot) override
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

        // Render this slot without any vertex colours
        glDisableClientState(GL_COLOR_ARRAY);

        glFrontFace(GL_CW);

        auto& slotInfo = _slots.at(slot);
        auto& buffer = getBufferByIndex(slotInfo.bufferIndex);

        buffer.renderSurface(slotInfo.surfaceIndex);

        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    IGeometryStore::Slot getStorageLocation(Slot slot) override
    {
        return _slots.at(slot).storageHandle;
    }

private:
    constexpr static std::uint8_t GetBufferIndexForIndexType(GeometryType indexType)
    {
        return static_cast<std::uint8_t>(indexType);
    }

    VertexBuffer& getBufferByIndex(std::uint8_t bufferIndex)
    {
        return _buffers[bufferIndex];
    }

    Slot getNextFreeSlotMapping()
    {
        auto numSlots = _slots.size();

        for (auto i = _freeSlotMappingHint; i < numSlots; ++i)
        {
            if (_slots[i].surfaceIndex == InvalidSurfaceIndex)
            {
                _freeSlotMappingHint = i + 1; // start searching here next time
                return i;
            }
        }

        _slots.emplace_back();
        return numSlots; // == the size before we emplaced the new slot
    }
};

}
