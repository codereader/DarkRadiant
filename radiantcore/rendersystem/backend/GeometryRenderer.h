#pragma once

#include "igeometryrenderer.h"
#include "igeometrystore.h"
#include "ObjectRenderer.h"

namespace render
{

/**
 * Maintains all surfaces grouped by the GeometryType (Triangles, Lines, etc.)
 * Stores the vertex data in the IGeometryStore provided by the render backend.
 */
class GeometryRenderer :
    public IGeometryRenderer
{
private:
    // Manages all surfaces drawn in a certain GL mode like GL_TRIANGLES, GL_QUADS, etc.
    class SurfaceGroup
    {
    private:
        IGeometryStore& _store;
        GLenum _mode;

        std::set<IGeometryStore::Slot> _surfaces;

    public:
        SurfaceGroup(IGeometryStore& store, GLenum mode) :
            _store(store),
            _mode(mode)
        {}

        bool empty() const
        {
            return _surfaces.empty();
        }

        void renderAll()
        {
            ObjectRenderer::SubmitGeometry(_surfaces, _mode, _store);
        }

        void renderSurface(IGeometryStore::Slot slot) const
        {
            ObjectRenderer::SubmitGeometry(slot, _mode, _store);
        }

        // Returns the surface index within this buffer
        IGeometryStore::Slot addSurface(const std::vector<ArbitraryMeshVertex>& vertices,
            const std::vector<unsigned int>& indices)
        {
            // Save the data into the backend storage
            auto slot = _store.allocateSlot(vertices, indices);

            _surfaces.insert(slot);

            return slot;
        }

        void updateSurface(IGeometryStore::Slot slot,
            const std::vector<ArbitraryMeshVertex>& vertices,
            const std::vector<unsigned int>& indices)
        {
            _store.updateData(slot, vertices, indices);
        }

        void removeSurface(IGeometryStore::Slot slot)
        {
            _store.deallocateSlot(slot);
            _surfaces.erase(slot);
        }

        AABB getSurfaceBounds(IGeometryStore::Slot slot)
        {
            return _store.getBounds(slot);
        }
    };

    std::vector<SurfaceGroup> _groups;

    static constexpr IGeometryStore::Slot InvalidStorageHandle = std::numeric_limits<IGeometryStore::Slot>::max();

    // Internal information about where the chunk of indexed vertex data is located:
    // Which buffer they're in, and the data offset and count within the buffer.
    // This is enough information to access, replace or remove the data at a later point.
    struct SlotInfo
    {
        std::uint8_t groupIndex;
        IGeometryStore::Slot storageHandle;
    };
    std::vector<SlotInfo> _slots;

    static constexpr std::size_t InvalidSlotMapping = std::numeric_limits<std::size_t>::max();
    std::size_t _freeSlotMappingHint;

public:
    GeometryRenderer(IGeometryStore& store) :
        _freeSlotMappingHint(InvalidSlotMapping)
    {
        _groups.emplace_back(store, GL_TRIANGLES);
        _groups.emplace_back(store, GL_QUADS);
        _groups.emplace_back(store, GL_LINES);
        _groups.emplace_back(store, GL_POINTS);
    }

    bool empty() const
    {
        for (const auto& buffer : _groups)
        {
            if (!buffer.empty()) return false;
        }

        return true;
    }

    Slot addGeometry(GeometryType indexType, const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        auto groupIndex = GetGroupIndexForIndexType(indexType);
        auto& buffer = getGroupByIndex(groupIndex);

        // Allocate a slot
        auto newSlotIndex = getNextFreeSlotMapping();
        auto& slot = _slots.at(newSlotIndex);

        slot.storageHandle = buffer.addSurface(vertices, indices);
        slot.groupIndex = groupIndex;

        return newSlotIndex;
    }

    void removeGeometry(Slot slot) override
    {
        auto& slotInfo = _slots.at(slot);
        auto& buffer = getGroupByIndex(slotInfo.groupIndex);

        buffer.removeSurface(slotInfo.storageHandle);

        // Invalidate the slot
        slotInfo.storageHandle = InvalidStorageHandle;
        
        if (slot < _freeSlotMappingHint)
        {
            _freeSlotMappingHint = slot;
        }
    }

    void updateGeometry(Slot slot, const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        auto& slotInfo = _slots.at(slot);
        auto& buffer = getGroupByIndex(slotInfo.groupIndex);

        buffer.updateSurface(slotInfo.storageHandle, vertices, indices);
    }

    AABB getGeometryBounds(Slot slot) override
    {
        auto& slotInfo = _slots.at(slot);
        auto& group = getGroupByIndex(slotInfo.groupIndex);

        return group.getSurfaceBounds(slotInfo.storageHandle);
    }

    void render()
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        glFrontFace(GL_CW);

        for (auto& buffer : _groups)
        {
            buffer.renderAll();
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
        auto& buffer = getGroupByIndex(slotInfo.groupIndex);

        buffer.renderSurface(slotInfo.storageHandle);

        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    IGeometryStore::Slot getGeometryStorageLocation(Slot slot) override
    {
        return _slots.at(slot).storageHandle;
    }

private:
    constexpr static std::uint8_t GetGroupIndexForIndexType(GeometryType indexType)
    {
        return static_cast<std::uint8_t>(indexType);
    }

    SurfaceGroup& getGroupByIndex(std::uint8_t groupIndex)
    {
        return _groups[groupIndex];
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
