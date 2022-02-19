#pragma once

#include "irender.h"
#include "igeometryrenderer.h"
#include "igeometrystore.h"

namespace render
{

class SurfaceRenderer :
    public ISurfaceRenderer
{
private:
    IGeometryStore& _store;

    static constexpr IGeometryStore::Slot InvalidStorageSlot = std::numeric_limits<IGeometryStore::Slot>::max();

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

    void render(const VolumeTest& view, const RenderInfo& info)
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

        // Render this slot without any vertex colours
        glDisableClientState(GL_COLOR_ARRAY);

        glFrontFace(GL_CW);

        for (auto& surface : _surfaces)
        {
            renderSlot(surface.second, &view, info.checkFlag(RENDER_BUMP));
        }

        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    void renderSurface(Slot slot) override
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

        // Render this slot without any vertex colours
        glDisableClientState(GL_COLOR_ARRAY);

        glFrontFace(GL_CW);

        renderSlot(_surfaces.at(slot));

        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    IGeometryStore::Slot getSurfaceStorageLocation(ISurfaceRenderer::Slot slot) override
    {
        return _surfaces.at(slot).storageHandle;
    }

private:
    void renderSlot(SurfaceInfo& slot, const VolumeTest* view = nullptr, bool renderBump = false)
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

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

        glMultMatrixd(surface.getObjectTransform());

        auto renderParams = _store.getRenderParameters(slot.storageHandle);

        glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->vertex);
        glColorPointer(4, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->colour);

        if (renderBump)
        {
            glVertexAttribPointer(GLProgramAttribute::Normal, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->normal);
            glVertexAttribPointer(GLProgramAttribute::TexCoord, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->texcoord);
            glVertexAttribPointer(GLProgramAttribute::Tangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->tangent);
            glVertexAttribPointer(GLProgramAttribute::Bitangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->bitangent);
        }
        else
        {
            glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->texcoord);
            glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &renderParams.bufferStart->normal);
        }

        glDrawElementsBaseVertex(GL_TRIANGLES, static_cast<GLsizei>(renderParams.indexCount), 
            GL_UNSIGNED_INT, renderParams.firstIndex, static_cast<GLint>(renderParams.firstVertex));

        glPopMatrix();
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
