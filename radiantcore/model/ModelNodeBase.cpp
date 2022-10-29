#include "ModelNodeBase.h"

namespace model
{

ModelNodeBase::ModelNodeBase() :
    _attachedToShaders(false)
{}

scene::INode::Type ModelNodeBase::getNodeType() const
{
    return Type::Model;
}

void ModelNodeBase::onPreRender(const VolumeTest& volume)
{
    assert(_renderEntity);

    // Attach renderables (or do nothing if everything is up to date)
    attachToShaders();
}

void ModelNodeBase::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    auto identity = Matrix4::getIdentity();

    for (const auto& surface : _renderableSurfaces)
    {
        collector.addHighlightRenderable(*surface, identity);
    }
}

std::size_t ModelNodeBase::getHighlightFlags()
{
    return Highlight::NoHighlight; // models are never highlighted themselves
}

void ModelNodeBase::onInsertIntoScene(scene::IMapRootNode& root)
{
    // Renderables will acquire their shaders in onPreRender
    createRenderableSurfaces();

    Node::onInsertIntoScene(root);
}

void ModelNodeBase::onRemoveFromScene(scene::IMapRootNode& root)
{
    destroyRenderableSurfaces();

    Node::onRemoveFromScene(root);
}

void ModelNodeBase::destroyRenderableSurfaces()
{
    detachFromShaders();

    _renderableSurfaces.clear();
}

void ModelNodeBase::detachFromShaders()
{
    // Detach any existing surfaces. In case we need them again,
    // the node will re-attach in the next pre-render phase
    for (auto& surface : _renderableSurfaces)
    {
        surface->detach();
    }

    _attachedToShaders = false;
}

void ModelNodeBase::attachToShaders()
{
    // Refuse to attach without a render entity
    if (_attachedToShaders || !_renderEntity) return;

    auto renderSystem = _renderSystem.lock();

    if (!renderSystem) return;

    for (auto& surface : _renderableSurfaces)
    {
        auto shader = renderSystem->capture(surface->getSurface().getActiveMaterial());

        // Skip filtered materials
        //TODO if (!shader->isVisible()) continue;

        // Solid mode
        surface->attachToShader(shader);

        // For orthoview rendering we need the entity's wireframe shader
        surface->attachToShader(_renderEntity->getWireShader());

        // Attach to the render entity for lighting mode rendering
        surface->attachToEntity(_renderEntity, shader);
    }

    _attachedToShaders = true;
}

void ModelNodeBase::queueRenderableUpdate()
{
    for (auto& surface : _renderableSurfaces)
    {
        surface->queueUpdate();
    }
}

void ModelNodeBase::transformChangedLocal()
{
    Node::transformChangedLocal();

    for (auto& surface : _renderableSurfaces)
    {
        surface->boundsChanged();
    }
}

void ModelNodeBase::onVisibilityChanged(bool isVisibleNow)
{
    if (isVisibleNow)
    {
        attachToShaders();
    }
    else
    {
        detachFromShaders();
    }
}

}
