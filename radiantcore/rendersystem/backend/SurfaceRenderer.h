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

    std::vector<Slot> _surfacesNeedingUpdate;
    bool _surfacesNeedUpdate;

public:
    SurfaceRenderer(IGeometryStore& store) :
        _store(store),
        _freeSlotMappingHint(0),
        _surfacesNeedUpdate(false)
    {}

    bool empty() const
    {
        return _surfaces.empty();
    }

    Slot addSurface(IRenderableSurface& surface) override
    {
        // Find a free slot
        auto newSlotIndex = getNextFreeSlotIndex();

        const auto& vertices = surface.getVertices();
        const auto& indices = surface.getIndices();

        auto slot = _store.allocateSlot(vertices.size(), indices.size());

        // Transform the vertices to single precision
        _store.updateData(slot, ConvertToRenderVertices(vertices), indices);

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
        _surfacesNeedingUpdate.push_back(slot);
        _surfacesNeedUpdate = true;
    }

    void render(const VolumeTest& view)
    {
        for (auto& surface : _surfaces)
        {
            renderSlot(surface.second, false, &view);
        }
    }

    void renderSurface(Slot slot) override
    {
        renderSlot(_surfaces.at(slot), true);
    }

    IGeometryStore::Slot getSurfaceStorageLocation(ISurfaceRenderer::Slot slot) override
    {
        return _surfaces.at(slot).storageHandle;
    }

    // Ensures the data in the IGeometryStore is up to date
    void prepareForRendering()
    {
        if (!_surfacesNeedUpdate) return;

        _surfacesNeedUpdate = false;

        for (auto slotIndex : _surfacesNeedingUpdate)
        {
            auto& surfaceInfo = _surfaces.at(slotIndex);

            if (surfaceInfo.surfaceDataChanged)
            {
                surfaceInfo.surfaceDataChanged = false;

                auto& surface = surfaceInfo.surface.get();
                _store.updateData(surfaceInfo.storageHandle, ConvertToRenderVertices(surface.getVertices()), surface.getIndices());
            }
        }
    }

private:
    std::vector<RenderVertex> ConvertToRenderVertices(const std::vector<MeshVertex>& vertices)
    {
        std::vector<RenderVertex> transformedVertices;
        transformedVertices.reserve(vertices.size());

        for (const auto& vertex : vertices)
        {
            transformedVertices.push_back(RenderVertex(vertex.vertex, vertex.normal, vertex.texcoord,
                vertex.colour, vertex.tangent, vertex.bitangent));
        }

        return transformedVertices;
    }

    void renderSlot(SurfaceInfo& slot, bool bindBuffer, const VolumeTest* view = nullptr)
    {
        auto& surface = slot.surface.get();

        // Check if this surface is in view
        if (view && view->TestAABB(surface.getObjectBounds(), surface.getObjectTransform()) == VOLUME_OUTSIDE)
        {
            return;
        }

        if (slot.surfaceDataChanged)
        {
            throw std::logic_error("Cannot render unprepared slot, ensure calling SurfaceRenderer::prepareForRendering first");
        }

        if (bindBuffer)
        {
            auto renderParams = _store.getRenderParameters(surface.getStorageLocation());

            auto [vertexBuffer, indexBuffer] = _store.getBufferObjects();
            vertexBuffer->bind();
            indexBuffer->bind();

            ObjectRenderer::InitAttributePointers(renderParams.bufferStart);
            ObjectRenderer::SubmitObject(surface, _store);

            vertexBuffer->unbind();
            indexBuffer->unbind();
        }
        else
        {
            ObjectRenderer::SubmitObject(surface, _store);
        }
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
