#include "MD5ModelNode.h"

#include "imodelcache.h"

namespace md5 {

// Local helper
inline void Surface_addLight(const MD5Surface& surface, 
								 VectorLightList& lights,
								 const Matrix4& localToWorld,
								 const RendererLight& light)
{
	if (light.testAABB(aabb_for_oriented_aabb(surface.localAABB(), localToWorld))) {
		lights.addLight(light);
	}
}

MD5ModelNode::MD5ModelNode(const MD5ModelPtr& model) :
	_model(model),
	_surfaceLightLists(_model->size()), 
	_surfaceRemaps(_model->size())
{
	_lightList = &GlobalRenderSystem().attach(*this);

	_model->_lightsChanged = LightsChangedCaller(*this);

	Node::setTransformChangedCallback(LightsChangedCaller(*this));

	constructRemaps();
}

const model::IModel& MD5ModelNode::getIModel() const {
	return *_model;
}

void MD5ModelNode::lightsChanged() {
	_lightList->lightsChanged();
}

MD5ModelNode::~MD5ModelNode() {
	destroyRemaps();
	GlobalRenderSystem().detach(*this);
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

void MD5ModelNode::instantiate(const scene::Path& path) {
	Node::instantiate(path);
}

void MD5ModelNode::uninstantiate(const scene::Path& path) {
	Node::uninstantiate(path);
}

// Cullable implementation
VolumeIntersectionValue MD5ModelNode::intersectVolume(
	const VolumeTest& test, const Matrix4& localToWorld) const
{
	return _model->intersectVolume(test, localToWorld);
}

std::string MD5ModelNode::name() const {
	return _model->getFilename();
}

void MD5ModelNode::testSelect(Selector& selector, SelectionTest& test) {
	_model->testSelect(selector, test, localToWorld());
}

bool MD5ModelNode::testLight(const RendererLight& light) const {
	return light.testAABB(worldAABB());
}

void MD5ModelNode::insertLight(const RendererLight& light) {
	const Matrix4& l2w = localToWorld();

	_surfaceLightLists.resize(_model->size());

	SurfaceLightLists::iterator j = _surfaceLightLists.begin();
	for (MD5Model::const_iterator i = _model->begin(); i != _model->end(); ++i) {
		Surface_addLight(*(*i), *j++, l2w, light);
	}
}

void MD5ModelNode::clearLights() {
	for (SurfaceLightLists::iterator i = _surfaceLightLists.begin(); 
		 i != _surfaceLightLists.end(); ++i)
	{
		i->clear();
	}
}

void MD5ModelNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const {
	_lightList->evaluateLights();

	render(collector, volume, localToWorld());
}

void MD5ModelNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const {
	renderSolid(collector, volume);
}

void MD5ModelNode::render(RenderableCollector& collector, const VolumeTest& volume,
		const Matrix4& localToWorld) const
{
	SurfaceLightLists::const_iterator j = _surfaceLightLists.begin();
	SurfaceRemaps::const_iterator k = _surfaceRemaps.begin();

	// greebo: Iterate over all MD5 surfaces and render them
	for (MD5Model::const_iterator i = _model->begin(); 
		 i != _model->end(); 
		 ++i, ++j, ++k)
	{
		if ((*i)->intersectVolume(volume, localToWorld) != c_volumeOutside) {
			collector.setLights(*j);
			(*i)->render(collector, localToWorld, k->shader != NULL ? k->shader : (*i)->getState());
		}
	}
}

void MD5ModelNode::constructRemaps() {
	// greebo: Acquire the ModelSkin reference from the SkinCache
	// Note: This always returns a valid reference
	ModelSkin& skin = GlobalModelSkinCache().capture(_skin);

	// Iterate over all surfaces and remaps
	_surfaceRemaps.resize(_model->size());
	MD5ModelNode::SurfaceRemaps::iterator j = _surfaceRemaps.begin();
	for (MD5Model::const_iterator i = _model->begin(); i != _model->end(); ++i,++j) {
		// Get the replacement shadername
		std::string remap = skin.getRemap((*i)->getShader());

		if (!remap.empty()) {
			// We have a valid remap, store it
			j->name = remap;
			j->shader = GlobalRenderSystem().capture(remap);
		} else {
			// No remap, leave the name as it is
			j->shader = ShaderPtr();
		}
	}

	// Refresh the scene
	GlobalSceneGraph().sceneChanged();
}

void MD5ModelNode::destroyRemaps() {
	// Iterate over all remaps and NULLify the shader pointers
	for (MD5ModelNode::SurfaceRemaps::iterator i = _surfaceRemaps.begin();
		 i != _surfaceRemaps.end(); ++i)
	{
		if (i->shader) {
			i->shader = ShaderPtr();
		}
	}
}

// Returns the name of the currently active skin
std::string MD5ModelNode::getSkin() const {
	return _skin;
}

void MD5ModelNode::skinChanged(const std::string& newSkinName) {
	ASSERT_MESSAGE(_surfaceRemaps.size() == _model->size(), "ERROR");
	destroyRemaps();

	// greebo: Store the new skin name locally
	_skin = newSkinName;

	constructRemaps();
}

/*void MD5ModelNode::refreshModel() {
	std::string modelPath = _model->getModelPath();
	
	// Acquire the model from the ModelCache
	model::IModelPtr freshModel = GlobalModelCache().getModel(modelPath);

	MD5ModelPtr freshMD5Model = 
		boost::dynamic_pointer_cast<MD5Model>(freshModel);

	if (freshMD5Model == NULL) {
		globalErrorStream() << "[MD5ModelNode]: Warning, could not refresh model"
			<< modelPath.c_str() << "\n";
	}

	// Overwrite the old model
	_model = freshMD5Model;

	// Refresh the shader mappings
	skinChanged(_skin);
}*/

} // namespace md5
