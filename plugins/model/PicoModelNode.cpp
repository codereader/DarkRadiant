#include "PicoModelNode.h"

#include "RenderablePicoSurface.h"
#include "ishaders.h"
#include "ifilter.h"
#include "imodelcache.h"
#include "math/frustum.h"

namespace model {

PicoModelNode::PicoModelNode(const RenderablePicoModelPtr& picoModel) :  
	_picoModel(picoModel),
	_name(picoModel->getFilename()),
	_lightList(GlobalShaderCache().attach(*this))
{
	Node::setTransformChangedCallback(LightsChangedCaller(*this));

	// Update the skin
	skinChanged("");
}

PicoModelNode::~PicoModelNode() {
	GlobalShaderCache().detach(*this);
}

const IModel& PicoModelNode::getIModel() const {
	return *_picoModel;
}

const AABB& PicoModelNode::localAABB() const {
	return _picoModel->localAABB();
}

VolumeIntersectionValue PicoModelNode::intersectVolume(
	const VolumeTest& test, const Matrix4& localToWorld) const
{
	return _picoModel->intersectVolume(test, localToWorld);
}

void PicoModelNode::instantiate(const scene::Path& path) {
	Node::instantiate(path);
}

void PicoModelNode::uninstantiate(const scene::Path& path) {
	Node::uninstantiate(path);
}

// SelectionTestable implementation
void PicoModelNode::testSelect(Selector& selector, SelectionTest& test) {
	_picoModel->testSelect(selector, test, localToWorld());
}

std::string PicoModelNode::name() const {
  	return _picoModel->getFilename();
}
  
const RenderablePicoModelPtr& PicoModelNode::getModel() const {
	return _picoModel;
}

void PicoModelNode::setModel(const RenderablePicoModelPtr& model) {
	_picoModel = model;
}

// LightCullable test function
bool PicoModelNode::testLight(const RendererLight& light) const {
	return light.testAABB(worldAABB());
}
	
// Add a light to this model instance
void PicoModelNode::insertLight(const RendererLight& light) {
	// Calculate transform from the superclass
	const Matrix4& l2w = localToWorld();
	
	// If the light's AABB intersects the oriented AABB of this model instance,
	// add the light to our light list
	if (light.testAABB(aabb_for_oriented_aabb(_picoModel->localAABB(),
											  l2w)))
	{
		_lights.addLight(light);
	}
}
	
// Clear all lights from this model instance
void PicoModelNode::clearLights() {
	_lights.clear();
}

void PicoModelNode::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	_lightList.evaluateLights();

	submitRenderables(renderer, volume, localToWorld());
}

void PicoModelNode::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	renderSolid(renderer, volume);
}

// Renderable submission
void PicoModelNode::submitRenderables(Renderer& renderer, 
									  const VolumeTest& volume, 
									  const Matrix4& localToWorld) const
{
	// Test the model's intersection volume, if it intersects pass on the 
	// render call
	if (_picoModel->intersectVolume(volume, localToWorld) != c_volumeOutside) {
		// Submit the lights
		renderer.setLights(_lights);
	
		// If the surface cache is populated, then use this instead of the
		// original model in order to get the skinned textures
		if (!_mappedSurfs.empty()) {
			for (MappedSurfaces::const_iterator i = _mappedSurfs.begin();
				 i != _mappedSurfs.end();
				 ++i)
			{
				// Submit the surface and shader to the renderer, checking first
				// to make sure the texture is not filtered
				IShaderPtr surfaceShader = i->second->getIShader();
				if (surfaceShader->isVisible()) { 
					renderer.SetState(i->second, Renderer::eFullMaterials);
					renderer.addRenderable(*i->first, localToWorld);
				}
			}			
		}
		else {
			// Submit the model's geometry
			_picoModel->submitRenderables(renderer, localToWorld);
		}
	}
}

// Skin changed notify
void PicoModelNode::skinChanged(const std::string& newSkinName) {

	// Clear all the surface mappings before doing anything
	_mappedSurfs.clear();

	// The new skin name is stored locally
	_skin = newSkinName;

	// greebo: Acquire the ModelSkin reference from the SkinCache
	// Note: This always returns a valid reference
	ModelSkin& skin = GlobalModelSkinCache().capture(_skin);

	// Otherwise get the list of RenderablePicoSurfaces from the model and
	// determine a texture remapping for each one
	const SurfaceList& surfs = _picoModel->getSurfaces();
	for (SurfaceList::const_iterator i = surfs.begin();
		 i != surfs.end();
		 ++i)
	{
		// Get the surface's material and test the skin for a remap
		std::string material = (*i)->getActiveMaterial();
		std::string mapped = skin.getRemap(material);
		if (mapped.empty())
			mapped = material; // use original material for remap
		
		// Add the surface and the mapped shader to our surface cache
		_mappedSurfs.push_back(
			std::make_pair(
				*i,
				GlobalShaderCache().capture(mapped)
			)
		);
	}
	
	// Refresh the scene
	GlobalSceneGraph().sceneChanged();
}

// Returns the name of the currently active skin
std::string PicoModelNode::getSkin() const {
	return _skin;
}

} // namespace model
