#pragma once

#include <vector>
#include "igeometryrenderer.h"
#include "isurfacerenderer.h"
#include "irenderableobject.h"
#include "irender.h"

namespace render
{

/**
 * Geometry base type, handling vertex data updates in combination 
 * with an IGeometryRenderer instance.
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

    std::size_t _lastVertexSize; // To detect size changes when updating geometry
    std::size_t _lastIndexSize; // To detect size changes when updating geometry

    class RenderAdapter final :
        public IRenderableObject
    {
    private:
        RenderableGeometry& _owner;
        AABB _bounds;
        bool _boundsNeedRecalculation;
        sigc::signal<void> _sigBoundsChanged;

    public:
        RenderAdapter(RenderableGeometry& owner) :
            _owner(owner),
            _boundsNeedRecalculation(true)
        {}

        bool isVisible() override
        {
            return _owner._surfaceSlot != IGeometryRenderer::InvalidSlot;
        }

        const Matrix4& getObjectTransform() override
        {
            static Matrix4 _identity = Matrix4::getIdentity();
            return _identity;
        }

        const AABB& getObjectBounds() override
        {
            if (_boundsNeedRecalculation)
            {
                _boundsNeedRecalculation = false;
                _bounds = _owner._shader->getGeometryBounds(_owner._surfaceSlot);
            }

            return _bounds;
        }

        // Will recalculate the geometry bounds next time getObjectBounds() is called
        // Also emits the signal to the observing entity
        void boundsChanged()
        {
            _boundsNeedRecalculation = true;
            signal_boundsChanged().emit();
        }

        sigc::signal<void>& signal_boundsChanged() override
        {
            return _sigBoundsChanged;
        }

        IGeometryStore::Slot getStorageLocation() override
        {
            if (_owner._surfaceSlot == IGeometryRenderer::InvalidSlot)
            {
                throw std::logic_error("Cannot access storage of unattached RenderableGeometry");
            }

            return _owner._shader->getGeometryStorageLocation(_owner._surfaceSlot);
        }
    };

    // Adapater suitable to be attached to an IRenderEntity
    std::shared_ptr<RenderAdapter> _renderAdapter;

    // The render entity the adapter is attached to
    IRenderEntity* _renderEntity;

protected:
    RenderableGeometry() :
        _surfaceSlot(IGeometryRenderer::InvalidSlot),
        _lastVertexSize(0),
        _lastIndexSize(0),
        _renderEntity(nullptr)
    {}

public:
    // Noncopyable
    RenderableGeometry(const RenderableGeometry& other) = delete;
    RenderableGeometry& operator=(const RenderableGeometry& other) = delete;

    virtual ~RenderableGeometry()
    {
        clear();
    }

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
    void clear()
    {
        // Detach from the render entity when being cleared
        detachFromEntity();

        removeGeometry();

        _shader.reset();
    }

    // Renders the geometry stored in our single slot
    void render(const RenderInfo& info) const override
    {
        if (_surfaceSlot != IGeometryRenderer::InvalidSlot && _shader)
        {
            _shader->renderGeometry(_surfaceSlot);
        }
    }

    // Attach this geometry to the given render entity.
    // This call is only valid if this Geometry instance has been attached to a shader
    // Does nothing if already attached to the given render entity.
    void attachToEntity(IRenderEntity* entity)
    {
        if (_renderEntity == entity) return; // nothing to do

        if (!_shader)
        {
            throw std::logic_error("Cannot attach geometry without any shader");
        }

        if (_renderEntity && entity != _renderEntity)
        {
            detachFromEntity();
        }

        _renderEntity = entity;
        ensureRenderAdapter();
        _renderEntity->addRenderable(_renderAdapter, _shader.get());
    }

    void detachFromEntity()
    {
        if (_renderEntity)
        {
            _renderEntity->removeRenderable(_renderAdapter);
            _renderEntity = nullptr;
        }
    }

protected:

    /**
     * Creates the adapter class suitable to attach this geometry as surface to a render entity
     * The surface will have an identity transform (all vertices specified in world coords).
     * This adapter will be valid as long as this geometry is attached to the IGeometryRenderer,
     * otherwise no access to the stored vertices/indices is possible.
     */
    void ensureRenderAdapter()
    {
        if (!_renderAdapter)
        {
            _renderAdapter = std::make_shared<RenderAdapter>(*this);
        }
    }

    // Removes the geometry from the attached shader. Does nothing if no geometry has been added.
    void removeGeometry()
    {
        if (_shader && _surfaceSlot != IGeometryRenderer::InvalidSlot)
        {
            _shader->removeGeometry(_surfaceSlot);
        }

        _lastVertexSize = 0;
        _lastIndexSize = 0;
        _surfaceSlot = IGeometryRenderer::InvalidSlot;
    }

    // Sub-class specific geometry update. Should check whether any of the vertex data 
    // needs to be added or updated to the shader, in which case the implementation 
    // should invoke the updateGeometry(type, vertices, indices) overload below
    virtual void updateGeometry() = 0;

    // Submits the given geometry to the known _shader reference
    // This method is supposed to be called from within updateGeometry()
    // to ensure that the _shader reference is already up to date.
    void updateGeometry(GeometryType type,
        const std::vector<ArbitraryMeshVertex>& vertices,
        const std::vector<unsigned int>& indices)
    {
        // Size changes require removal of the geometry before update
        if (_lastVertexSize != vertices.size() || _lastIndexSize != indices.size())
        {
            removeGeometry();

            _lastVertexSize = vertices.size();
            _lastIndexSize = indices.size();
        }

        if (vertices.empty() || indices.empty())
        {
            clear();
            return;
        }

        if (_surfaceSlot == IGeometryRenderer::InvalidSlot)
        {
            _surfaceSlot = _shader->addGeometry(type, vertices, indices);
        }
        else
        {
            _shader->updateGeometry(_surfaceSlot, vertices, indices);
        }

        // Fire the bounds changed signal (after submitting the changed vertices)
        if (_renderAdapter)
        {
            _renderAdapter->boundsChanged();
        }
    }
};

}
