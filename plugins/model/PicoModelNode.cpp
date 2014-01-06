#include "PicoModelNode.h"

#include "RenderablePicoSurface.h"
#include "ivolumetest.h"
#include "ishaders.h"
#include "iscenegraph.h"
#include "ifilter.h"
#include "imodelcache.h"
#include "math/Frustum.h"
#include "generic/callback.h"
#include <boost/bind.hpp>

namespace model {

// greebo: Construct a new RenderablePicoModel instance, we re-use the surfaces only
PicoModelNode::PicoModelNode(const RenderablePicoModelPtr& picoModel) :
	_picoModel(new RenderablePicoModel(*picoModel)), 
	_name(picoModel->getFilename()),
	_lightList(GlobalRenderSystem().attachLitObject(*this))
{
	Node::setTransformChangedCallback(boost::bind(&PicoModelNode::lightsChanged, this));

	// Update the skin
	skinChanged("");
}

PicoModelNode::~PicoModelNode() {
	GlobalRenderSystem().detachLitObject(*this);
}

const IModel& PicoModelNode::getIModel() const
{
	return *_picoModel;
}

IModel& PicoModelNode::getIModel() 
{
	return *_picoModel;
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

	submitRenderables(collector, volume, localToWorld(), *_renderEntity);
}

void PicoModelNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const {
	renderSolid(collector, volume);
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

// Renderable submission
void PicoModelNode::submitRenderables(RenderableCollector& collector,
									  const VolumeTest& volume,
									  const Matrix4& localToWorld,
									  const IRenderEntity& entity) const
{
	// Test the model's intersection volume, if it intersects pass on the
	// render call
	if (volume.TestAABB(_picoModel->localAABB(), localToWorld) != VOLUME_OUTSIDE)
    {
		// Submit the lights
		collector.setLights(_lights);

		// Submit the model's geometry
		_picoModel->submitRenderables(collector, localToWorld, entity);
	}
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

} // namespace model
