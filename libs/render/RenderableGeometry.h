#pragma once

#include "igeometryrenderer.h"
#include "irender.h"

namespace render
{

/**
 * Geometry base type, taking care of adding/removing/updating
 * indexed vertex data to an IGeometryRenderer instance.
 * 
 * It implements the OpenGLRenderable interface which will instruct
 * the shader to render just the geometry batch managed by this object.
 * This is used to render highlights (such as selection overlays).
 */
class RenderableGeometry :
    public OpenGLRenderable
{
protected:
    ShaderPtr _shader;
    IGeometryRenderer::Slot _surfaceSlot;

protected:
    RenderableGeometry() :
        _surfaceSlot(IGeometryRenderer::InvalidSlot)
    {}

public:
    // Removes any geometry attached to a shader
    virtual void clear()
    {
        if (_shader && _surfaceSlot != IGeometryRenderer::InvalidSlot)
        {
            _shader->removeGeometry(_surfaceSlot);
        }

        _shader.reset();
        _surfaceSlot = IGeometryRenderer::InvalidSlot;
    }

    virtual void render(const RenderInfo& info) const override
    {
        if (_surfaceSlot != IGeometryRenderer::InvalidSlot && _shader)
        {
            _shader->renderGeometry(_surfaceSlot);
        }
    }

protected:
    virtual void addOrUpdateGeometry(const ShaderPtr& shader, GeometryType type,
        const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices)
    {
        if (_surfaceSlot == IGeometryRenderer::InvalidSlot)
        {
            _surfaceSlot = shader->addGeometry(type, vertices, indices);
        }
        else
        {
            shader->updateGeometry(_surfaceSlot, vertices, indices);
        }
    }
};

}
