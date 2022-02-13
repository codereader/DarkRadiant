#include "StaticModelNode.h"

#include "StaticModelSurface.h"
#include "ivolumetest.h"
#include "ishaders.h"
#include "iscenegraph.h"
#include "ifilter.h"
#include "imap.h"
#include "imodelcache.h"
#include "math/Frustum.h"
#include "generic/callback.h"
#include <functional>

namespace model
{

StaticModelNode::StaticModelNode(const StaticModelPtr& picoModel) :
    _model(new StaticModel(*picoModel)),
    _name(picoModel->getFilename()),
    _attachedToShaders(false)
{
    // Update the skin
    skinChanged("");
}

void StaticModelNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    _model->connectUndoSystem(root.getUndoSystem());

    // Renderables will acquire their shaders in onPreRender
    _model->foreachSurface([&](const StaticModelSurface& surface)
    {
        if (surface.getVertexArray().empty() || surface.getIndexArray().empty())
        {
            return; // don't handle empty surfaces
        }

        _renderableSurfaces.emplace_back(std::make_shared<RenderableModelSurface>(surface, localToWorld()));
    });

    Node::onInsertIntoScene(root);
}

void StaticModelNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    _model->disconnectUndoSystem(root.getUndoSystem());

    for (auto& surface : _renderableSurfaces)
    {
        surface->detach();
    }

    _renderableSurfaces.clear();

    Node::onRemoveFromScene(root);
}

const IModel& StaticModelNode::getIModel() const
{
    return *_model;
}

IModel& StaticModelNode::getIModel()
{
    return *_model;
}

bool StaticModelNode::hasModifiedScale()
{
    return _model->getScale() != Vector3(1, 1, 1);
}

Vector3 StaticModelNode::getModelScale()
{
	return _model->getScale();
}

const AABB& StaticModelNode::localAABB() const {
    return _model->localAABB();
}

// SelectionTestable implementation
void StaticModelNode::testSelect(Selector& selector, SelectionTest& test) {
    _model->testSelect(selector, test, localToWorld());
}

std::string StaticModelNode::name() const {
    return _model->getFilename();
}

scene::INode::Type StaticModelNode::getNodeType() const
{
    return Type::Model;
}

const StaticModelPtr& StaticModelNode::getModel() const {
    return _model;
}

void StaticModelNode::setModel(const StaticModelPtr& model) {
    _model = model;
}

void StaticModelNode::onPreRender(const VolumeTest& volume)
{
    assert(_renderEntity);

    // Attach renderables (or do nothing if everything is up to date)
    attachToShaders();
}

void StaticModelNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    auto identity = Matrix4::getIdentity();

    for (const auto& surface : _renderableSurfaces)
    {
        collector.addHighlightRenderable(*surface, identity);
    }
}

void StaticModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    Node::setRenderSystem(renderSystem);

    _renderSystem = renderSystem;
    
    // Detach renderables on render system change
    detachFromShaders();

    _model->setRenderSystem(renderSystem);
}

void StaticModelNode::detachFromShaders()
{
    // Detach any existing surfaces. In case we need them again,
    // the node will re-attach in the next pre-render phase
    for (auto& surface : _renderableSurfaces)
    {
        surface->detach();
    }

    _attachedToShaders = false;
}

void StaticModelNode::attachToShaders()
{
    // Refuse to attach without a render entity
    if (_attachedToShaders || !_renderEntity) return;

    auto renderSystem = _renderSystem.lock();

    if (!renderSystem) return;

    for (auto& surface : _renderableSurfaces)
    {
        auto shader = renderSystem->capture(surface->getSurface().getActiveMaterial());
        
        // Solid mode
        surface->attachToShader(shader);

        // For orthoview rendering we need the entity's wireframe shader
        surface->attachToShader(_renderEntity->getWireShader());

        // Attach to the render entity for lighting mode rendering
        surface->attachToEntity(_renderEntity, shader);
    }

    _attachedToShaders = true;
}

void StaticModelNode::queueRenderableUpdate()
{
    for (auto& surface : _renderableSurfaces)
    {
        surface->queueUpdate();
    }
}

// Traceable implementation
bool StaticModelNode::getIntersection(const Ray& ray, Vector3& intersection)
{
    return _model->getIntersection(ray, intersection, localToWorld());
}

void StaticModelNode::transformChangedLocal()
{
    Node::transformChangedLocal();

    for (auto& surface : _renderableSurfaces)
    {
        surface->boundsChanged();
    }
}

// Skin changed notify
void StaticModelNode::skinChanged(const std::string& newSkinName)
{
    // The new skin name is stored locally
    _skin = newSkinName;

    // greebo: Acquire the ModelSkin reference from the SkinCache
    // Note: This always returns a valid reference
    ModelSkin& skin = GlobalModelSkinCache().capture(_skin);
    _model->applySkin(skin);

    // Detach from existing shaders, re-acquire them in onPreRender
    detachFromShaders();

    // Refresh the scene (TODO: get rid of that)
    GlobalSceneGraph().sceneChanged();
}

// Returns the name of the currently active skin
std::string StaticModelNode::getSkin() const
{
    return _skin;
}

void StaticModelNode::_onTransformationChanged()
{
    // Always revert to our original state before evaluating
    if (getTransformationType() & TransformationType::Scale)
    {
        _model->revertScale();
        _model->evaluateScale(getScale());
        queueRenderableUpdate();
    }
    else if (getTransformationType() == TransformationType::NoTransform)
    {
        // Transformation has been changed but no transform mode is set,
        // so the reason we got here is a cancelTransform() call, revert everything
        _model->revertScale();
        _model->evaluateScale(Vector3(1,1,1));
        queueRenderableUpdate();
    }
}

void StaticModelNode::_applyTransformation()
{
    if (getTransformationType() & TransformationType::Scale)
    {
        _model->revertScale();
        _model->evaluateScale(getScale());
        _model->freezeScale();
    }
}

void StaticModelNode::onVisibilityChanged(bool isVisibleNow)
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

} // namespace model
