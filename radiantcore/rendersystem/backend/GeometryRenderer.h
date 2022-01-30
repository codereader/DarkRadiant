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

        std::set<IGeometryStore::Slot> _surfaces;
        
#if 0
        // The input arrays used for glMultiDrawElementsBaseVertex
        // when drawing the entire buffer at once (non-lit mode)
        std::vector<GLsizei> _sizes;
        std::vector<void*> _firstIndices;
        std::vector<GLint> _firstVertices;
#endif

    public:
        VertexBuffer(IGeometryStore& store, GLenum mode) :
            _store(store),
            _mode(mode)
        {}

        bool empty() const
        {
            return _surfaces.empty();
        }

        void renderAll(bool renderBump)
        {
            auto surfaceCount = _surfaces.size();

            if (surfaceCount == 0) return;

            // Build the indices and offsets used for glMultiDraw
            std::vector<GLsizei> sizes;
            std::vector<void*> firstIndices;
            std::vector<GLint> firstVertices;

            sizes.reserve(surfaceCount);
            firstIndices.reserve(surfaceCount);
            firstVertices.reserve(surfaceCount);

            ArbitraryMeshVertex* bufferStart = nullptr;

            for (const auto slot : _surfaces)
            {
                auto renderParams = _store.getRenderParameters(slot);

                sizes.push_back(static_cast<GLsizei>(renderParams.indexCount));
                firstVertices.push_back(static_cast<GLint>(renderParams.firstVertex));
                firstIndices.push_back(renderParams.firstIndex);

                bufferStart = renderParams.bufferStart;
            }

            glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &bufferStart->vertex);
            glColorPointer(4, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &bufferStart->colour);

            if (renderBump)
            {
                glVertexAttribPointer(ATTR_NORMAL, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &bufferStart->normal);
                glVertexAttribPointer(ATTR_TEXCOORD, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &bufferStart->texcoord);
                glVertexAttribPointer(ATTR_TANGENT, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &bufferStart->tangent);
                glVertexAttribPointer(ATTR_BITANGENT, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &bufferStart->bitangent);
            }
            else
            {
                glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &bufferStart->texcoord);
                glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &bufferStart->normal);
            }

            glMultiDrawElementsBaseVertex(_mode, sizes.data(), GL_UNSIGNED_INT,
                &firstIndices.front(), static_cast<GLsizei>(sizes.size()), &firstVertices.front());
        }

        void renderSurface(IGeometryStore::Slot slot) const
        {
            auto renderParams = _store.getRenderParameters(slot);

            glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->vertex);
            glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->texcoord);
            glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->normal);

            glDrawElementsBaseVertex(_mode, static_cast<GLsizei>(renderParams.indexCount), GL_UNSIGNED_INT,
                renderParams.firstIndex, static_cast<GLint>(renderParams.firstVertex));
        }

        // Returns the surface index within this buffer
        IGeometryStore::Slot addSurface(const std::vector<ArbitraryMeshVertex>& vertices,
            const std::vector<unsigned int>& indices)
        {
            // Save the data into the backend storage
            auto slot = _store.allocateSlot(vertices, indices);

            _surfaces.insert(slot);

            return slot;
#if 0
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
#endif
        }

        void updateSurface(IGeometryStore::Slot slot,
            const std::vector<ArbitraryMeshVertex>& vertices,
            const std::vector<unsigned int>& indices)
        {
            _store.updateData(slot, vertices, indices);
#if 0
            auto firstVertex = _firstVertices.at(surfaceIndex);

            // Calculate the distance within the index buffer from the pointer difference
            auto* surfaceStartIndex = static_cast<unsigned int*>(_firstIndices.at(surfaceIndex));
            auto firstIndex = surfaceStartIndex - &_indices.front();

            // Copy the data to the correct slot in the array
            std::copy(vertices.begin(), vertices.end(), _vertices.begin() + firstVertex);
            std::copy(indices.begin(), indices.end(), _indices.begin() + firstIndex);
#endif
        }

        void removeSurface(IGeometryStore::Slot slot)
        {
            _store.deallocateSlot(slot);
            _surfaces.erase(slot);
#if 0
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
#endif
        }
    };

    std::vector<VertexBuffer> _buffers;

    static constexpr IGeometryStore::Slot InvalidStorageHandle = std::numeric_limits<IGeometryStore::Slot>::max();

    // Internal information about where the chunk of indexed vertex data is located:
    // Which buffer they're in, and the data offset and count within the buffer.
    // This is enough information to access, replace or remove the data at a later point.
    struct SlotInfo
    {
        std::uint8_t bufferIndex;
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

        slot.storageHandle = buffer.addSurface(vertices, indices);

        slot.bufferIndex = bufferIndex;
        slot.numVertices = vertices.size();
        slot.numIndices = indices.size();

        return newSlotIndex;
    }

    void removeGeometry(Slot slot) override
    {
        auto& slotInfo = _slots.at(slot);
        auto& buffer = getBufferByIndex(slotInfo.bufferIndex);

        buffer.removeSurface(slotInfo.storageHandle);

#if 0
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
#endif
        // Invalidate the slot
        slotInfo.numVertices = 0;
        slotInfo.storageHandle = InvalidStorageHandle;
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

        auto& buffer = getBufferByIndex(slotInfo.bufferIndex);

        buffer.updateSurface(slotInfo.storageHandle, vertices, indices);
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
            buffer.renderAll(info.checkFlag(RENDER_BUMP));
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

        buffer.renderSurface(slotInfo.storageHandle);

        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    IGeometryStore::Slot getGeometryStorageLocation(Slot slot) override
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
            if (_slots[i].storageHandle == InvalidStorageHandle)
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
