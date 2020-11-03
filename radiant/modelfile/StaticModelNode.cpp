#include "StaticModelNode.h"

#include "StaticModelSurface.h"
#include "ivolumetest.h"
#include "ishaders.h"
#include "iscenegraph.h"
#include "ifilter.h"
#include "imodelcache.h"
#include "math/Frustum.h"
#include "generic/callback.h"
#include <functional>

namespace model {

// greebo: Construct a new StaticModel instance, we re-use the surfaces only
StaticModelNode::StaticModelNode(const StaticModelPtr& picoModel) :
    _picoModel(new StaticModel(*picoModel)), 
    _name(picoModel->getFilename()),
    _lightList(GlobalRenderSystem().attachLitObject(*this))
{
    Node::setTransformChangedCallback(std::bind(&StaticModelNode::lightsChanged, this));

    // Update the skin
    skinChanged("");
}

StaticModelNode::~StaticModelNode() {
    GlobalRenderSystem().detachLitObject(*this);
}

void StaticModelNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    _picoModel->connectUndoSystem(root.getUndoChangeTracker());
    
    Node::onInsertIntoScene(root);
}

void StaticModelNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    _picoModel->disconnectUndoSystem(root.getUndoChangeTracker());

    Node::onRemoveFromScene(root);
}

const IModel& StaticModelNode::getIModel() const
{
    return *_picoModel;
}

IModel& StaticModelNode::getIModel() 
{
    return *_picoModel;
}

bool StaticModelNode::hasModifiedScale()
{
    return _picoModel->getScale() != Vector3(1, 1, 1);
}

Vector3 StaticModelNode::getModelScale()
{
	return _picoModel->getScale();
}

const AABB& StaticModelNode::localAABB() const {
    return _picoModel->localAABB();
}

// SelectionTestable implementation
void StaticModelNode::testSelect(Selector& selector, SelectionTest& test) {
    _picoModel->testSelect(selector, test, localToWorld());
}

std::string StaticModelNode::name() const {
    return _picoModel->getFilename();
}

scene::INode::Type StaticModelNode::getNodeType() const
{
    return Type::Model;
}

const StaticModelPtr& StaticModelNode::getModel() const {
    return _picoModel;
}

void StaticModelNode::setModel(const StaticModelPtr& model) {
    _picoModel = model;
}

// LitObject test function
bool StaticModelNode::intersectsLight(const RendererLight& light) const
{
    return light.intersectsAABB(worldAABB());
}

// Add a light to this model instance
void StaticModelNode::insertLight(const RendererLight& light)
{
    // Calculate transform from the superclass
    const Matrix4& l2w = localToWorld();

    // If the light's AABB intersects the oriented AABB of this model instance,
    // add the light to our light list
    if (light.intersectsAABB(AABB::createFromOrientedAABB(_picoModel->localAABB(), l2w)))
    {
        _intersectingLights.addLight(light);
    }
}

// Clear all lights from this model instance
void StaticModelNode::clearLights()
{
    _intersectingLights.clear();
}

void StaticModelNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
    _lightList.calculateIntersectingLights();

    assert(_renderEntity);

    // Test the model's intersection volume, if it intersects pass on the render call
    const Matrix4& l2w = localToWorld();

    if (volume.TestAABB(_picoModel->localAABB(), l2w) != VOLUME_OUTSIDE)
    {
        // Submit the model's geometry
        _picoModel->renderSolid(collector, l2w, *_renderEntity,
                                _intersectingLights);
    }
}

void StaticModelNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
    assert(_renderEntity);

    // Test the model's intersection volume, if it intersects pass on the render call
    const Matrix4& l2w = localToWorld();

    if (volume.TestAABB(_picoModel->localAABB(), l2w) != VOLUME_OUTSIDE)
    {
        // Submit the model's geometry
        _picoModel->renderWireframe(collector, l2w, *_renderEntity);
    }
}

void StaticModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    Node::setRenderSystem(renderSystem);

    _picoModel->setRenderSystem(renderSystem);
}

// Traceable implementation
bool StaticModelNode::getIntersection(const Ray& ray, Vector3& intersection)
{
    return _picoModel->getIntersection(ray, intersection, localToWorld());
}

// Skin changed notify
void StaticModelNode::skinChanged(const std::string& newSkinName)
{
    // The new skin name is stored locally
    _skin = newSkinName;

    // greebo: Acquire the ModelSkin reference from the SkinCache
    // Note: This always returns a valid reference
    ModelSkin& skin = GlobalModelSkinCache().capture(_skin);
    _picoModel->applySkin(skin);

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
        _picoModel->revertScale();
        _picoModel->evaluateScale(getScale());
    }
    else if (getTransformationType() == TransformationType::NoTransform)
    {
        // Transformation has been changed but no transform mode is set,
        // so the reason we got here is a cancelTransform() call, revert everything
        _picoModel->revertScale();
        _picoModel->evaluateScale(Vector3(1,1,1));
    }
}

void StaticModelNode::_applyTransformation()
{
    if (getTransformationType() & TransformationType::Scale)
    {
        _picoModel->revertScale();
        _picoModel->evaluateScale(getScale());
        _picoModel->freezeScale();
    }
}

} // namespace model
