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
private:
    ShaderPtr _shader;
    IGeometryRenderer::Slot _surfaceSlot;

protected:
    RenderableGeometry() :
        _surfaceSlot(IGeometryRenderer::InvalidSlot)
    {}

public:
    // Noncopyable
    RenderableGeometry(const RenderableGeometry& other) = delete;
    RenderableGeometry& operator=(const RenderableGeometry& other) = delete;

    // (Non-virtual) update method handling any possible shader change
    // The geometry is withdrawn from the given shader if it turns out
    // to be different from the last update.
    void update(const ShaderPtr& shader)
    {
        bool shaderChanged = _shader != shader;

        if (shaderChanged)
        {
            clear();
        }

        // Update our local shader reference
        _shader = shader;

        if (_shader)
        {
            // Invoke the virtual method to run needed updates in the subclass
            updateGeometry();
        }
    }

    // Removes the geometry and clears the shader reference
    virtual void clear()
    {
        removeGeometry();

        _shader.reset();
    }

    virtual void render(const RenderInfo& info) const override
    {
        if (_surfaceSlot != IGeometryRenderer::InvalidSlot && _shader)
        {
            _shader->renderGeometry(_surfaceSlot);
        }
    }

protected:
    // Removes the geometry from the attached shader. Does nothing if no geometry has been added.
    void removeGeometry()
    {
        if (_shader && _surfaceSlot != IGeometryRenderer::InvalidSlot)
        {
            _shader->removeGeometry(_surfaceSlot);
        }

        _surfaceSlot = IGeometryRenderer::InvalidSlot;
    }

    // Sub-class specific geometry update. Should check whether any of the
    // vertex data needs to be added or updated to the shader, in which case
    // the implementation should invoke addOrUpdateGeometry()
    virtual void updateGeometry() = 0;

    // Submits the given geometry to the known _shader reference
    // This method is supposed to be called from within updateGeometry()
    // to ensure that the _shader reference is already up to date.
    virtual void addOrUpdateGeometry(GeometryType type,
        const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices)
    {
        if (_surfaceSlot == IGeometryRenderer::InvalidSlot)
        {
            _surfaceSlot = _shader->addGeometry(type, vertices, indices);
        }
        else
        {
            _shader->updateGeometry(_surfaceSlot, vertices, indices);
        }
    }
};

}
