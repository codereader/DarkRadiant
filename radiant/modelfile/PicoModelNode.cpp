#include "PicoModelNode.h"

#include "RenderablePicoSurface.h"
#include "ivolumetest.h"
#include "ishaders.h"
#include "iscenegraph.h"
#include "ifilter.h"
#include "imodelcache.h"
#include "math/Frustum.h"
#include "generic/callback.h"
#include <functional>

namespace model {

// greebo: Construct a new RenderablePicoModel instance, we re-use the surfaces only
PicoModelNode::PicoModelNode(const RenderablePicoModelPtr& picoModel) :
	_picoModel(new RenderablePicoModel(*picoModel)), 
	_name(picoModel->getFilename()),
	_lightList(GlobalRenderSystem().attachLitObject(*this))
{
	Node::setTransformChangedCallback(std::bind(&PicoModelNode::lightsChanged, this));

	// Update the skin
	skinChanged("");
}

PicoModelNode::~PicoModelNode() {
	GlobalRenderSystem().detachLitObject(*this);
}

void PicoModelNode::onInsertIntoScene(scene::IMapRootNode& root)
{
	_picoModel->connectUndoSystem(root.getUndoChangeTracker());
	
	Node::onInsertIntoScene(root);
}

void PicoModelNode::onRemoveFromScene(scene::IMapRootNode& root)
{
	_picoModel->disconnectUndoSystem(root.getUndoChangeTracker());

	Node::onRemoveFromScene(root);
}

const IModel& PicoModelNode::getIModel() const
{
	return *_picoModel;
}

IModel& PicoModelNode::getIModel() 
{
	return *_picoModel;
}

bool PicoModelNode::hasModifiedScale()
{
	return _picoModel->getScale() != Vector3(1, 1, 1);
}

const AABB& PicoModelNode::localAABB() const {
	return _picoModel->localAABB();
}

// SelectionTestable implementation
void PicoModelNode::testSelect(Selector& selector, SelectionTest& test) {
	_picoModel->testSelect(selector, test, localToWorld());
}

std::string PicoModelNode::name() const {
  	return _picoModel->getFilename();
}

scene::INode::Type PicoModelNode::getNodeType() const
{
	return Type::Model;
}

const RenderablePicoModelPtr& PicoModelNode::getModel() const {
	return _picoModel;
}

void PicoModelNode::setModel(const RenderablePicoModelPtr& model) {
	_picoModel = model;
}

// LitObject test function
bool PicoModelNode::intersectsLight(const RendererLight& light) const
{
	return light.intersectsAABB(worldAABB());
}

// Add a light to this model instance
void PicoModelNode::insertLight(const RendererLight& light)
{
	// Calculate transform from the superclass
	const Matrix4& l2w = localToWorld();

	// If the light's AABB intersects the oriented AABB of this model instance,
	// add the light to our light list
	if (light.intersectsAABB(AABB::createFromOrientedAABB(_picoModel->localAABB(), l2w)))
	{
		_lights.addLight(light);
	}
}

// Clear all lights from this model instance
void PicoModelNode::clearLights()
{
	_lights.clear();
}

void PicoModelNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	_lightList.calculateIntersectingLights();

	assert(_renderEntity);

	// Test the model's intersection volume, if it intersects pass on the render call
	const Matrix4& l2w = localToWorld();

	if (volume.TestAABB(_picoModel->localAABB(), l2w) != VOLUME_OUTSIDE)
	{
		// Submit the model's geometry
		_picoModel->renderSolid(collector, l2w, *_renderEntity, _lights);
	}
}

void PicoModelNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
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

void PicoModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	Node::setRenderSystem(renderSystem);

	_picoModel->setRenderSystem(renderSystem);
}

// Traceable implementation
bool PicoModelNode::getIntersection(const Ray& ray, Vector3& intersection)
{
	return _picoModel->getIntersection(ray, intersection, localToWorld());
}

// Skin changed notify
void PicoModelNode::skinChanged(const std::string& newSkinName)
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
std::string PicoModelNode::getSkin() const
{
	return _skin;
}

void PicoModelNode::_onTransformationChanged()
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

void PicoModelNode::_applyTransformation()
{
	if (getTransformationType() & TransformationType::Scale)
	{
		_picoModel->revertScale();
		_picoModel->evaluateScale(getScale());
		_picoModel->freezeScale();
	}
}

} // namespace model
