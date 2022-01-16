#pragma once

#include <vector>
#include "irender.h"
#include "Winding.h"

namespace render
{

class RenderableWinding :
    public OpenGLRenderable
{
private:
    const Winding& _winding;
    ShaderPtr _shader;
    bool _needsUpdate;

    IWindingRenderer::Slot _slot;
    Winding::size_type _windingSize;

public:
    RenderableWinding(const Winding& winding) :
        _winding(winding),
        _needsUpdate(true),
        _slot(IWindingRenderer::InvalidSlot),
        _windingSize(0)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

    void update(const ShaderPtr& shader, const IRenderEntity& entity)
    {
        bool shaderChanged = _shader != shader;

        if (!_needsUpdate && !shaderChanged) return;

        _needsUpdate = false;

        auto numPoints = _winding.size();

        if (numPoints < 3 || !shader)
        {
            clear();
            return;
        }

        std::vector<ArbitraryMeshVertex> vertices;
        vertices.reserve(numPoints);

        // Use the colour defined by the entity as vertex colour
        auto entityColour = entity.getEntityColour();

        for (const auto& vertex : _winding)
        {
            vertices.emplace_back(ArbitraryMeshVertex(vertex.vertex, vertex.normal, vertex.texcoord, entityColour));
        }

        if (_shader && _slot != IWindingRenderer::InvalidSlot && (shaderChanged || numPoints != _windingSize))
        {
            _shader->removeWinding(_slot);
            _slot = IWindingRenderer::InvalidSlot;
            _windingSize = 0;
        }

        _shader = shader;
        _windingSize = numPoints;

        if (_slot == IWindingRenderer::InvalidSlot)
        {
            _slot = shader->addWinding(vertices);
        }
        else
        {
            shader->updateWinding(_slot, vertices);
        }
    }

    void clear()
    {
        if (!_shader || _slot == IWindingRenderer::InvalidSlot) return;

        _shader->removeWinding(_slot);
        _shader.reset();
        _slot = IWindingRenderer::InvalidSlot;
        _windingSize = 0;
    }

    void render(const RenderInfo& info) const override
    {
        if (_slot != render::IWindingRenderer::InvalidSlot && _shader)
        {
            _shader->renderWinding(render::IWindingRenderer::RenderMode::Polygon, _slot);
        }
    }
};

}
