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
    IRenderEntity* _entity;
    bool _needsUpdate;

    IWindingRenderer::Slot _slot;
    Winding::size_type _windingSize;

    bool _useEntityColourAsVertexColour;
public:
    // Set useEntityColourAsVertexColour to false to set all vertex colours to white
    RenderableWinding(const Winding& winding, bool useEntityColourAsVertexColour) :
        _winding(winding),
        _entity(nullptr),
        _needsUpdate(true),
        _slot(IWindingRenderer::InvalidSlot),
        _windingSize(0),
        _useEntityColourAsVertexColour(useEntityColourAsVertexColour)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

    void update(const ShaderPtr& shader, IRenderEntity& entity)
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        if (_shader != shader || &entity != _entity)
        {
            clear();
        }

        auto numPoints = _winding.size();

        if (numPoints < 3 || !shader)
        {
            clear();
            return;
        }

        std::vector<RenderVertex> vertices;
        vertices.reserve(numPoints);

        // Either use the colour defined by the entity as vertex colour, or use plain white
        auto entityColour = _useEntityColourAsVertexColour ? entity.getEntityColour() : Vector4(1,1,1,1);

        for (const auto& vertex : _winding)
        {
            vertices.emplace_back(RenderVertex(vertex.vertex, vertex.normal, 
                vertex.texcoord, entityColour, vertex.tangent, vertex.bitangent));
        }

        if (_shader && _slot != IWindingRenderer::InvalidSlot && numPoints != _windingSize)
        {
            _shader->removeWinding(_slot);
            _slot = IWindingRenderer::InvalidSlot;
            _windingSize = 0;
        }

        _shader = shader;
        _windingSize = numPoints;
        _entity = &entity; // remember this renderentity, to detect colour changes

        if (_slot == IWindingRenderer::InvalidSlot)
        {
            _slot = shader->addWinding(vertices, _entity);
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

    void render() const override
    {
        if (_slot != render::IWindingRenderer::InvalidSlot && _shader)
        {
            _shader->renderWinding(render::IWindingRenderer::RenderMode::Polygon, _slot);
        }
    }
};

}
