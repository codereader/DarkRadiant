#pragma once

#include <map>
#include <stdexcept>
#include "irender.h"
#include "isurfacerenderer.h"
#include "igeometrystore.h"

#include "ObjectRenderer.h"

namespace render
{

class SurfaceRenderer :
    public ISurfaceRenderer
{
private:
    IGeometryStore& _store;

    struct SurfaceInfo
    {
        std::reference_wrapper<IRenderableSurface> surface;
        bool surfaceDataChanged;
        IGeometryStore::Slot storageHandle;

        SurfaceInfo(IRenderableSurface& surface_, IGeometryStore::Slot slot) :
            surface(surface_),
            surfaceDataChanged(false),
            storageHandle(slot)
        {}
    };
    std::map<Slot, SurfaceInfo> _surfaces;

    Slot _freeSlotMappingHint;

public:
    SurfaceRenderer(IGeometryStore& store) :
        _store(store),
        _freeSlotMappingHint(0)
    {}

    bool empty() const
    {
        return _surfaces.empty();
    }

    Slot addSurface(IRenderableSurface& surface) override
    {
        // Find a free slot
        auto newSlotIndex = getNextFreeSlotIndex();

        auto slot = _store.allocateSlot(surface.getVertices(), surface.getIndices());
        _surfaces.emplace(newSlotIndex, SurfaceInfo(surface, slot));

        return newSlotIndex;
    }

    void removeSurface(Slot slot) override
    {
        auto surface = _surfaces.find(slot);
        assert(surface != _surfaces.end());

        // Deallocate the storage
        _store.deallocateSlot(surface->second.storageHandle);
        _surfaces.erase(surface);

        if (slot < _freeSlotMappingHint)
        {
            _freeSlotMappingHint = slot;
        }
    }

    void updateSurface(Slot slot) override
    {
        _surfaces.at(slot).surfaceDataChanged = true;
    }

    void render(const VolumeTest& view)
    {
        for (auto& surface : _surfaces)
        {
            renderSlot(surface.second, &view);
        }
    }

    void renderSurface(Slot slot) override
    {
        renderSlot(_surfaces.at(slot));
    }

    IGeometryStore::Slot getSurfaceStorageLocation(ISurfaceRenderer::Slot slot) override
    {
        return _surfaces.at(slot).storageHandle;
    }

private:
    void renderSlot(SurfaceInfo& slot, const VolumeTest* view = nullptr)
    {
        auto& surface = slot.surface.get();

        // Check if this surface is in view
        if (view && view->TestAABB(surface.getObjectBounds(), surface.getObjectTransform()) == VOLUME_OUTSIDE)
        {
            return;
        }

        // Update the vertex data now if necessary
        if (slot.surfaceDataChanged)
        {
            slot.surfaceDataChanged = false;

            _store.updateData(slot.storageHandle, surface.getVertices(), surface.getIndices());
        }

        ObjectRenderer::SubmitObject(surface, _store);
    }

    Slot getNextFreeSlotIndex()
    {
        for (auto i = _freeSlotMappingHint; i < std::numeric_limits<Slot>::max(); ++i)
        {
            if (_surfaces.count(i) == 0)
            {
                _freeSlotMappingHint = i + 1; // start searching here next time
                return i;
            }
        }
        
        throw std::runtime_error("SurfaceRenderer ran out of surface slot numbers");
    }
};

}
