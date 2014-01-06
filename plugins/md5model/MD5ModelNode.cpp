#include "MD5ModelNode.h"

#include "ivolumetest.h"
#include "imodelcache.h"
#include "ishaders.h"
#include "iscenegraph.h"
#include <boost/bind.hpp>

namespace md5 {

// Local helper
inline void Surface_addLight(const MD5Surface& surface,
                             render::lib::VectorLightList& lights,
                             const Matrix4& localToWorld,
                             const RendererLight& light)
{
	if (light.intersectsAABB(
            AABB::createFromOrientedAABB(surface.localAABB(), localToWorld)
        )
    )
    {
		lights.addLight(light);
	}
}

MD5ModelNode::MD5ModelNode(const MD5ModelPtr& model) :
	_model(new MD5Model(*model)), // create a copy of the incoming model, we need our own instance
	_surfaceLightLists(_model->size())
{
	_lightList = &GlobalRenderSystem().attachLitObject(*this);

	Node::setTransformChangedCallback(Callback((boost::bind(&MD5ModelNode::lightsChanged, this))));
}

const model::IModel& MD5ModelNode::getIModel() const {
	return *_model;
}

model::IModel& MD5ModelNode::getIModel() {
	return *_model;
}

void MD5ModelNode::lightsChanged()
{
	_lightList->setDirty();
}

MD5ModelNode::~MD5ModelNode()
{
	GlobalRenderSystem().detachLitObject(*this);
}

void MD5ModelNode::setModel(const MD5ModelPtr& model) {
	_model = model;
}

const MD5ModelPtr& MD5ModelNode::getModel() const {
	return _model;
}

// Bounded implementation
const AABB& MD5ModelNode::localAABB() const {
	return _model->localAABB();
}

std::string MD5ModelNode::name() const {
	return _model->getFilename();
}

scene::INode::Type MD5ModelNode::getNodeType() const
{
	return Type::Model;
}

void MD5ModelNode::testSelect(Selector& selector, SelectionTest& test) {
	_model->testSelect(selector, test, localToWorld());
}

bool MD5ModelNode::getIntersection(const Ray& ray, Vector3& intersection)
{
	return _model->getIntersection(ray, intersection, localToWorld());
}

bool MD5ModelNode::intersectsLight(const RendererLight& light) const
{
	return light.intersectsAABB(worldAABB());
}

void MD5ModelNode::insertLight(const RendererLight& light) {
	const Matrix4& l2w = localToWorld();

	_surfaceLightLists.resize(_model->size());

	SurfaceLightLists::iterator j = _surfaceLightLists.begin();
	for (MD5Model::const_iterator i = _model->begin(); i != _model->end(); ++i) {
		Surface_addLight(*i->surface, *j++, l2w, light);
	}
}

void MD5ModelNode::clearLights() {
	for (SurfaceLightLists::iterator i = _surfaceLightLists.begin();
		 i != _surfaceLightLists.end(); ++i)
	{
		i->clear();
	}
}

void MD5ModelNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	_lightList->calculateIntersectingLights();

	assert(_renderEntity);

	render(collector, volume, localToWorld(), *_renderEntity);
}

void MD5ModelNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	renderSolid(collector, volume);
}

void MD5ModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	Node::setRenderSystem(renderSystem);

	_model->setRenderSystem(renderSystem);
}

void MD5ModelNode::render(RenderableCollector& collector, const VolumeTest& volume,
		const Matrix4& localToWorld, const IRenderEntity& entity) const
{
	// Do some rough culling (per model, not per surface)
	if (volume.TestAABB(localAABB(), localToWorld) == VOLUME_OUTSIDE)
	{
		return;
	}

	SurfaceLightLists::const_iterator j = _surfaceLightLists.begin();

	// greebo: Iterate over all MD5 surfaces and render them
	for (MD5Model::const_iterator i = _model->begin();
		 i != _model->end();
		 ++i, ++j)
	{
		assert(i->shader);

		// Get the Material to test the shader name against the filter system
		const MaterialPtr& surfaceShader = i->shader->getMaterial();

		if (surfaceShader->isVisible())
		{
			collector.setLights(*j);
			i->surface->render(collector, localToWorld, i->shader, entity);
		}
	}

	// Uncomment to render the skeleton
	//collector.SetState(entity.getWireShader(), RenderableCollector::eFullMaterials);
	//collector.addRenderable(_model->getRenderableSkeleton(), localToWorld, entity);
}

// Returns the name of the currently active skin
std::string MD5ModelNode::getSkin() const {
	return _skin;
}

void MD5ModelNode::skinChanged(const std::string& newSkinName)
{
	// greebo: Store the new skin name locally
	_skin = newSkinName;

	// greebo: Acquire the ModelSkin reference from the SkinCache
	// Note: This always returns a valid reference
	ModelSkin& skin = GlobalModelSkinCache().capture(_skin);

	_model->applySkin(skin);

	// Refresh the scene
	GlobalSceneGraph().sceneChanged();
}

} // namespace md5
