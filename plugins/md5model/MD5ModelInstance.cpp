#include "MD5ModelInstance.h"

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

MD5ModelInstance::MD5ModelInstance(const scene::Path& path, scene::Instance* parent, MD5Model& model) :
	Instance(path, parent), _model(model),
	_surfaceLightLists(_model.size()), _surfaceRemaps(_model.size())
{
	_lightList = &GlobalShaderCache().attach(*this);
	_model._lightsChanged = LightsChangedCaller(*this);

	Instance::setTransformChangedCallback(LightsChangedCaller(*this));

	constructRemaps();
}

MD5ModelInstance::~MD5ModelInstance() {
	destroyRemaps();

	Instance::setTransformChangedCallback(Callback());

	_model._lightsChanged = Callback();
	GlobalShaderCache().detach(*this);
}

// Bounded implementation
const AABB& MD5ModelInstance::localAABB() const {
	return _model.localAABB();
}

// Cullable implementation
VolumeIntersectionValue MD5ModelInstance::intersectVolume(
	const VolumeTest& test, const Matrix4& localToWorld) const
{
	return _model.intersectVolume(test, localToWorld);
}

void MD5ModelInstance::lightsChanged() {
	_lightList->lightsChanged();
}

void MD5ModelInstance::constructRemaps() {
	// greebo: Acquire the ModelSkin reference from the SkinCache
	// Note: This always returns a valid reference
	ModelSkin& skin = GlobalModelSkinCache().capture(_skin);

	// Iterate over all surfaces and remaps
	SurfaceRemaps::iterator j = _surfaceRemaps.begin();
	for (MD5Model::const_iterator i = _model.begin(); i != _model.end(); ++i,++j) {
		// Get the replacement shadername
		std::string remap = skin.getRemap((*i)->getShader());

		if (!remap.empty()) {
			// We have a valid remap, store it
			j->name = remap;
			j->shader = GlobalShaderCache().capture(remap);
		} else {
			// No remap, leave the name as it is
			j->shader = ShaderPtr();
		}
	}

	// Refresh the scene
	GlobalSceneGraph().sceneChanged();
}

void MD5ModelInstance::destroyRemaps() {
	// Iterate over all remaps and NULLify the shader pointers
	for (SurfaceRemaps::iterator i = _surfaceRemaps.begin(); i
			!= _surfaceRemaps.end(); ++i) {
		if (i->shader) {
			i->shader = ShaderPtr();
		}
	}
}

// Returns the name of the currently active skin
std::string MD5ModelInstance::getSkin() const {
	return _skin;
}

void MD5ModelInstance::skinChanged(const std::string& newSkinName) {
	ASSERT_MESSAGE(_surfaceRemaps.size() == _model.size(), "ERROR");
	destroyRemaps();

	// greebo: Store the new skin name locally
	_skin = newSkinName;

	constructRemaps();
}

void MD5ModelInstance::render(Renderer& renderer, const VolumeTest& volume,
		const Matrix4& localToWorld) const {
	SurfaceLightLists::const_iterator j = _surfaceLightLists.begin();
	SurfaceRemaps::const_iterator k = _surfaceRemaps.begin();

	// greebo: Iterate over all MD5 surfaces and render them
	for (MD5Model::const_iterator i = _model.begin(); 
		 i != _model.end(); 
		 ++i, ++j, ++k)
	{
		if ((*i)->intersectVolume(volume, localToWorld) != c_volumeOutside) {
			renderer.setLights(*j);
			(*i)->render(renderer, localToWorld, k->shader != NULL ? k->shader : (*i)->getState());
		}
	}
}

void MD5ModelInstance::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	_lightList->evaluateLights();

	render(renderer, volume, Instance::localToWorld());
}
void MD5ModelInstance::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	renderSolid(renderer, volume);
}

void MD5ModelInstance::testSelect(Selector& selector, SelectionTest& test) {
	_model.testSelect(selector, test, Instance::localToWorld());
}

bool MD5ModelInstance::testLight(const RendererLight& light) const {
	return light.testAABB(worldAABB());
}

void MD5ModelInstance::insertLight(const RendererLight& light) {
	const Matrix4& localToWorld = Instance::localToWorld();
	SurfaceLightLists::iterator j = _surfaceLightLists.begin();
	for (MD5Model::const_iterator i = _model.begin(); i != _model.end(); ++i) {
		Surface_addLight(*(*i), *j++, localToWorld, light);
	}
}

void MD5ModelInstance::clearLights() {
	for (SurfaceLightLists::iterator i = _surfaceLightLists.begin(); i
			!= _surfaceLightLists.end(); ++i) {
		(*i).clear();
	}
}

} // namespace md5
