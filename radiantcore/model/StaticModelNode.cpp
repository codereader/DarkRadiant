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
        _renderableSurfaces.emplace_back(std::make_shared<RenderableStaticSurface>(surface, localToWorld()));
    });

    Node::onInsertIntoScene(root);
}

void StaticModelNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    _model->disconnectUndoSystem(root.getUndoSystem());

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

bool StaticModelNode::isOriented() const
{
    return false;
}

void StaticModelNode::onPreRender(const VolumeTest& volume)
{
    assert(_renderEntity);

    // Attach renderables (or do nothing if everything is up to date)
    attachToShaders();
}

void StaticModelNode::renderSolid(IRenderableCollector& collector, const VolumeTest& volume) const
{
    assert(_renderEntity);

    const Matrix4& l2w = localToWorld();

    // The space partitioning system will consider this node also if the cell is only partially visible
    // Do a quick bounds check against the world AABB to cull ourselves if we're not in the view
    if (volume.TestAABB(worldAABB()) != VOLUME_OUTSIDE)
    {
#if 0
        // Submit the model's geometry
        _model->renderSolid(collector, l2w, *_renderEntity, *this);
#endif
    }
}

void StaticModelNode::renderWireframe(IRenderableCollector& collector, const VolumeTest& volume) const
{
    assert(_renderEntity);

#if 0
    // Submit the model's geometry
    _model->renderWireframe(collector, localToWorld(), *_renderEntity);
#endif
}

void StaticModelNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
#if 0
    if (collector.supportsFullMaterials())
    {
        renderSolid(collector, volume);
    }
    else
    {
        renderWireframe(collector, volume);
    }
#endif
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
        surface->clear();
    }

    _attachedToShaders = false;
}

void StaticModelNode::attachToShaders()
{
    if (_attachedToShaders) return;

    _attachedToShaders = true;

    auto renderSystem = _renderSystem.lock();

    for (auto& surface : _renderableSurfaces)
    {
        auto shader = renderSystem->capture(surface->getSurface().getActiveMaterial());
        surface->attachToShader(shader);

        // For orthoview rendering we need the entity's colour shader
        if (_renderEntity)
        {
            surface->attachToShader(_renderEntity->getColourShader());
        }
    }
}

// Traceable implementation
bool StaticModelNode::getIntersection(const Ray& ray, Vector3& intersection)
{
    return _model->getIntersection(ray, intersection, localToWorld());
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
    }
    else if (getTransformationType() == TransformationType::NoTransform)
    {
        // Transformation has been changed but no transform mode is set,
        // so the reason we got here is a cancelTransform() call, revert everything
        _model->revertScale();
        _model->evaluateScale(Vector3(1,1,1));
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

} // namespace model
