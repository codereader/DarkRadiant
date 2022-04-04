#pragma once

#include <map>
#include <vector>
#include "isurfacerenderer.h"
#include "irender.h"

namespace render
{

/**
 * Surface base type, handling vertex data updates in combination
 * with one or more ISurfaceRenderer instances.
 *
 * It implements the OpenGLRenderable interface which will instruct
 * the shader to render just the surface managed by this object.
 * This is used to render highlights (such as selection overlays).
 */
class RenderableSurface :
    public IRenderableSurface,
    public OpenGLRenderable,
    public std::enable_shared_from_this<RenderableSurface>
{
private:
    using ShaderMapping = std::map<ShaderPtr, ISurfaceRenderer::Slot>;
    ShaderMapping _shaders;

    sigc::signal<void> _sigBoundsChanged;

    // The render entity the adapter is attached to
    IRenderEntity* _renderEntity;

    // When attached to an entity, this is the backend storage handle
    IGeometryStore::Slot _storageLocation;

protected:
    RenderableSurface() :
        _renderEntity(nullptr),
        _storageLocation(std::numeric_limits<IGeometryStore::Slot>::max())
    {}

public:
    // Noncopyable
    RenderableSurface(const RenderableSurface& other) = delete;
    RenderableSurface& operator=(const RenderableSurface& other) = delete;

    virtual ~RenderableSurface()
    {
        detach();
    }

    // (Non-virtual) update method handling any possible shader change
    // The surface is withdrawn from the given shader if it turns out
    // to be different from the last update.
    void attachToShader(const ShaderPtr& shader)
    {
        if (_shaders.count(shader) > 0)
        {
            return; // already attached
        }

        _shaders[shader] = shader->addSurface(*this);
    }

    // Notifies all the attached shaders that the surface geometry changed
    // Also fires the bounds changed signal
    void queueUpdate()
    {
        for (auto& [shader, slot] : _shaders)
        {
            shader->updateSurface(slot);
        }

        boundsChanged();
    }

    void boundsChanged()
    {
        _sigBoundsChanged.emit();
    }

    // Removes the surface from all shaders
    void detach()
    {
        // Detach from the render entity when being cleared
        detachFromEntity();

        while (!_shaders.empty())
        {
            detachFromShader(_shaders.begin());
        }
    }

    // Attach this geometry to the given render entity.
    // This call is only valid if this instance has been attached to the shader first
    // Does nothing if already attached to the given render entity.
    void attachToEntity(IRenderEntity* entity, const ShaderPtr& shader)
    {
        assert(_shaders.count(shader) > 0);

        if (_renderEntity == entity) return; // nothing to do

        if (_renderEntity && entity != _renderEntity)
        {
            detachFromEntity();
        }

        _renderEntity = entity;
        _renderEntity->addRenderable(shared_from_this(), shader.get());
        _storageLocation = shader->getSurfaceStorageLocation(_shaders[shader]);
    }

    // Renders the surface stored in our single slot
    void render() const override
    {
        if (_shaders.empty()) return;

        auto& [shader, slot] = *_shaders.begin();
        shader->renderSurface(slot);
    }

    sigc::signal<void>& signal_boundsChanged() override
    {
        return _sigBoundsChanged;
    }

    IGeometryStore::Slot getStorageLocation() override
    {
        assert(_storageLocation != std::numeric_limits<IGeometryStore::Slot>::max());
        return _storageLocation;
    }

private:
    void detachFromEntity()
    {
        if (_renderEntity)
        {
            _renderEntity->removeRenderable(shared_from_this());
            _renderEntity = nullptr;
        }

        _storageLocation = std::numeric_limits<IGeometryStore::Slot>::max();
    }

    void detachFromShader(const ShaderMapping::iterator& iter)
    {
        iter->first->removeSurface(iter->second);
        _shaders.erase(iter);
    }
};

}
