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

void ModelNodeBase::emplaceRenderableSurface(RenderableModelSurface::Ptr&& surface)
{
    _renderableSurfaces.emplace_back(std::move(surface));
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
        auto shader = surface->captureFillShader(*renderSystem);

        // Skip filtered materials - the wireframe shader itself is always visible
        // so filtered surfaces need to be kept from attaching their geometry
        if (!shader->isVisible()) continue;

        // Solid mode
        surface->attachToShader(shader);

        // For orthoview rendering we need a wireframe shader
        surface->attachToShader(getRenderState() == RenderState::Active ? 
            surface->captureWireShader(*renderSystem) : _inactiveShader);

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
    Node::onVisibilityChanged(isVisibleNow);

    if (isVisibleNow)
    {
        attachToShaders();
    }
    else
    {
        detachFromShaders();
    }
}

void ModelNodeBase::onFiltersChanged()
{
    Node::onFiltersChanged();

    // When filters change, some of our surfaces might come into view or end up hidden
    // Detach existing surfaces, the next rendering pass will rebuild them
    detachFromShaders();
}

void ModelNodeBase::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    Node::setRenderSystem(renderSystem);

    if (renderSystem)
    {
        _inactiveShader = renderSystem->capture(BuiltInShaderType::WireframeInactive);
    }
    else
    {
        _inactiveShader.reset();
    }
}

void ModelNodeBase::onRenderStateChanged()
{
    Node::onRenderStateChanged();

    detachFromShaders();
}

}
