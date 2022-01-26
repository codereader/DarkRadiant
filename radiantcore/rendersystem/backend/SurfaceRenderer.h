#pragma once

#include "irender.h"
#include "igeometryrenderer.h"

namespace render
{

class SurfaceRenderer :
    public ISurfaceRenderer
{
private:
    class VertexBuffer
    {
    private:
        GLenum _mode;

        std::vector<ArbitraryMeshVertex> _vertices;
        std::vector<unsigned int> _indices;
    public:
        VertexBuffer(GLenum mode) :
            _mode(mode)
        {}

        bool empty() const
        {
            return _indices.empty();
        }

        void clear()
        {
            _vertices.clear();
            _indices.clear();
        }

        void render(bool renderBump) const
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

            glDrawElements(_mode, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, &_indices.front());
        }

        void renderIndexRange(std::size_t firstIndex, std::size_t numIndices) const
        {
            glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().vertex);
            glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().texcoord);
            glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &_vertices.front().normal);

            glDrawElements(_mode, static_cast<GLsizei>(numIndices), GL_UNSIGNED_INT, &_indices[firstIndex]);
        }

        // Returns the vertex and index offsets in this buffer
        std::pair<std::size_t, std::size_t> addSurface(const std::vector<ArbitraryMeshVertex>& vertices,
            const std::vector<unsigned int>& indices)
        {
            auto vertexOffset = _vertices.size();
            auto indexOffset = _indices.size();

            std::copy(vertices.begin(), vertices.end(), std::back_inserter(_vertices));

            for (auto index : indices)
            {
                _indices.push_back(index + static_cast<unsigned int>(vertexOffset));
            }

            return { vertexOffset, indexOffset };
        }

        void updateSurface(std::size_t firstVertex, std::size_t firstIndex,
            const std::vector<ArbitraryMeshVertex>& vertices,
            const std::vector<unsigned int>& indices)
        {
            // Copy the data to the correct slot in the array
            std::copy(vertices.begin(), vertices.end(), _vertices.begin() + firstVertex);

            // Before assignment, the indices need to be shifted to match the array offset of the vertices
            auto targetIndex = _indices.begin() + firstIndex;
            auto indexShift = static_cast<unsigned int>(firstVertex);

            for (auto index : indices)
            {
                *targetIndex++ = index + indexShift;
            }
        }

        // Cuts out the vertices and indices, adjusts all indices located to the right of the cut
        void removeSurface(std::size_t firstVertex, std::size_t numVertices, std::size_t firstIndex, std::size_t numIndices)
        {
            // Cut out the vertices
            auto firstVertexToRemove = _vertices.begin() + firstVertex;
            _vertices.erase(firstVertexToRemove, firstVertexToRemove + numVertices);

            // Shift all indices to the left, offsetting their values by the number of removed vertices
            auto offsetToApply = -static_cast<int>(numVertices);

            auto targetIndex = _indices.begin() + firstIndex;
            auto indexToMove = targetIndex + numIndices;

            auto indexEnd = _indices.end();
            while (indexToMove != indexEnd)
            {
                *targetIndex++ = *indexToMove++ + offsetToApply;
            }

            // Cut off the tail of the indices
            _indices.resize(_indices.size() - numIndices);
        }
    };

    struct SurfaceInfo
    {
        std::reference_wrapper<IRenderableSurface> surface;
        bool surfaceDataChanged;
        VertexBuffer buffer;

        SurfaceInfo(IRenderableSurface& surface_) :
            surface(surface_),
            buffer(GL_TRIANGLES),
            surfaceDataChanged(true)
        {}
    };
    std::map<Slot, SurfaceInfo> _surfaces;

    Slot _freeSlotMappingHint;

public:
    SurfaceRenderer() :
        _freeSlotMappingHint(0)
    {}

    bool empty() const
    {
        return _surfaces.empty();
    }

    Slot addSurface(IRenderableSurface& surface, IRenderEntity* entity) override
    {
        // Find a free slot
        auto newSlotIndex = getNextFreeSlotIndex();

        _surfaces.emplace(newSlotIndex, surface);

        return newSlotIndex;
    }

    void removeSurface(Slot slot) override
    {
        // Remove the surface
        _surfaces.erase(slot);

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

private:
    void renderSlot(SurfaceInfo& slot, const VolumeTest* view = nullptr, bool renderBump = false)
    {
        auto& surface = slot.surface.get();

        // Update the vertex data now if necessary
        if (slot.surfaceDataChanged)
        {
            slot.surfaceDataChanged = false;

            slot.buffer.clear();
            slot.buffer.addSurface(surface.getVertices(), surface.getIndices());
        }

        if (slot.buffer.empty())
        {
            return;
        }

        // Check if this surface is in view
        if (view && view->TestAABB(surface.getSurfaceBounds(), surface.getSurfaceTransform()) == VOLUME_OUTSIDE)
        {
            return;
        }

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

        glMultMatrixd(surface.getSurfaceTransform());

        slot.buffer.render(renderBump);

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
