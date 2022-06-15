#pragma once

#include "igeometryrenderer.h"
#include "igeometrystore.h"
#include "iobjectrenderer.h"

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
    // Contains all geometry sharing the same GL primitive mode like GL_TRIANGLES, GL_QUADS, etc.
    struct SurfaceGroup
    {
        GLenum primitiveMode;
        std::set<IGeometryStore::Slot> visibleStorageHandles;
        
        SurfaceGroup(GLenum mode) :
            primitiveMode(mode)
        {}
    };

    IGeometryStore& _store;
    IObjectRenderer& _renderer;

    std::vector<SurfaceGroup> _groups;

    static constexpr IGeometryStore::Slot InvalidStorageHandle = std::numeric_limits<IGeometryStore::Slot>::max();

    // Internal mapping to the respective group and the storage location
    struct SlotInfo
    {
        std::uint8_t groupIndex;
        IGeometryStore::Slot storageHandle;
    };
    std::vector<SlotInfo> _slots;

    static constexpr std::size_t InvalidSlotMapping = std::numeric_limits<std::size_t>::max();
    std::size_t _freeSlotMappingHint;

public:
    GeometryRenderer(IGeometryStore& store, IObjectRenderer& renderer) :
        _store(store),
        _renderer(renderer),
        _freeSlotMappingHint(InvalidSlotMapping)
    {
        // Must be the same order as in render::GeometryType
        _groups.emplace_back(GL_TRIANGLES);
        _groups.emplace_back(GL_QUADS);
        _groups.emplace_back(GL_LINES);
        _groups.emplace_back(GL_POINTS);

        // Check we're getting the order right
        assert(getGroupByIndex(GetGroupIndexForIndexType(GeometryType::Triangles)).primitiveMode == GL_TRIANGLES);
        assert(getGroupByIndex(GetGroupIndexForIndexType(GeometryType::Quads)).primitiveMode == GL_QUADS);
        assert(getGroupByIndex(GetGroupIndexForIndexType(GeometryType::Lines)).primitiveMode == GL_LINES);
        assert(getGroupByIndex(GetGroupIndexForIndexType(GeometryType::Points)).primitiveMode == GL_POINTS);
    }

    bool empty() const
    {
        for (const auto& group : _groups)
        {
            if (!group.visibleStorageHandles.empty()) return false;
        }

        return true;
    }

    Slot addGeometry(GeometryType indexType, const std::vector<RenderVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        auto groupIndex = GetGroupIndexForIndexType(indexType);
        auto& group = getGroupByIndex(groupIndex);

        // Allocate a slot
        auto newSlotIndex = getNextFreeSlotMapping();
        auto& slot = _slots.at(newSlotIndex);

        // Save the data into the backend storage
        slot.storageHandle = _store.allocateSlot(vertices.size(), indices.size());
        _store.updateData(slot.storageHandle, vertices, indices);

        // New geometry is automatically added to the visible set
        group.visibleStorageHandles.insert(slot.storageHandle);

        slot.groupIndex = groupIndex;

        return newSlotIndex;
    }

    void activateGeometry(Slot slot) override
    {
        const auto& slotInfo = _slots.at(slot);
        auto& group = getGroupByIndex(slotInfo.groupIndex);

        // Remove the geometry from the visible set
        group.visibleStorageHandles.insert(slotInfo.storageHandle);
    }

    void deactivateGeometry(Slot slot) override
    {
        const auto& slotInfo = _slots.at(slot);
        auto& group = getGroupByIndex(slotInfo.groupIndex);

        // Remove the geometry from the visible set
        group.visibleStorageHandles.erase(slotInfo.storageHandle);
    }

    void removeGeometry(Slot slot) override
    {
        auto& slotInfo = _slots.at(slot);
        auto& group = getGroupByIndex(slotInfo.groupIndex);

        // Release the memory in the geometry store
        _store.deallocateSlot(slotInfo.storageHandle);

        // Remove the geometry from the visible set
        group.visibleStorageHandles.erase(slotInfo.storageHandle);

        // Invalidate the slot
        slotInfo.storageHandle = InvalidStorageHandle;
        
        if (slot < _freeSlotMappingHint)
        {
            _freeSlotMappingHint = slot;
        }
    }

    void updateGeometry(Slot slot, const std::vector<RenderVertex>& vertices,
        const std::vector<unsigned int>& indices) override
    {
        const auto& slotInfo = _slots.at(slot);

        // Upload the new vertex and index data
        _store.updateData(slotInfo.storageHandle, vertices, indices);
    }

    AABB getGeometryBounds(Slot slot) override
    {
        const auto& slotInfo = _slots.at(slot);

        return _store.getBounds(slotInfo.storageHandle);
    }

    void renderAllVisibleGeometry() override
    {
        for (auto& group : _groups)
        {
            if (group.visibleStorageHandles.empty()) continue;

            _renderer.submitGeometry(group.visibleStorageHandles, group.primitiveMode);
        }
    }

    void renderGeometry(Slot slot) override
    {
        const auto& slotInfo = _slots.at(slot);
        const auto& group = getGroupByIndex(slotInfo.groupIndex);

        _renderer.submitGeometry(slotInfo.storageHandle, group.primitiveMode);
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
