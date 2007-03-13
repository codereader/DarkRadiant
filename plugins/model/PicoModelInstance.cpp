#include "PicoModelInstance.h"
#include "RenderablePicoSurface.h"

#include "math/frustum.h"

namespace model
{

// Main constructor
PicoModelInstance::PicoModelInstance(const scene::Path& path, 
									 scene::Instance* parent, 
									 RenderablePicoModel& picomodel)
: Instance(path, parent, this, StaticTypeCasts::instance().get()),
  _picoModel(picomodel),
  _lightList(GlobalShaderCache().attach(*this))
{ 
	Instance::setTransformChangedCallback(LightsChangedCaller(*this));
	
	// Update the skin
	skinChanged();
}

// Destructor
PicoModelInstance::~PicoModelInstance() {
    Instance::setTransformChangedCallback(Callback());
	GlobalShaderCache().detach(*this);
}

// Skin changed notify
void PicoModelInstance::skinChanged() {

	// Get the model skin object from the parent entity and clear out our list
	// of mapped surfaces
	ModelSkin* skin = NodeTypeCast<ModelSkin>::cast(path().parent());
	_mappedSurfs.clear();
	
	// If skin is null, return
	if (skin == NULL)
		return;
		
	// Otherwise get the list of RenderablePicoSurfaces from the model and
	// determine a texture remapping for each one
	SurfaceList surfs = _picoModel.getSurfaces();
	for (SurfaceList::const_iterator i = surfs.begin();
		 i != surfs.end();
		 ++i)
	{
		// Get the surface's material and test the skin for a remap
		std::string material = (*i)->getActiveMaterial();
		std::string mapped = skin->getRemap(material);
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

// Renderable submission
void PicoModelInstance::submitRenderables(Renderer& renderer, 
										  const VolumeTest& volume, 
										  const Matrix4& localToWorld) const
{
	// Test the model's intersection volume, if it intersects pass on the 
	// render call
	if (_picoModel.intersectVolume(volume, localToWorld) 
		!= c_volumeOutside)
	{
		// Submit the lights
		renderer.setLights(_lights);
	
		// If the surface cache is populated, then use this instead of the
		// original model in order to get the skinned textures
		if (!_mappedSurfs.empty()) {
			for (MappedSurfaces::const_iterator i = _mappedSurfs.begin();
				 i != _mappedSurfs.end();
				 ++i)
			{
				// Submit the surface and shader to the renderer
				renderer.SetState(i->second, Renderer::eFullMaterials);
				renderer.addRenderable(*(i->first), localToWorld);
			}			
		}
		else {
			// Submit the model's geometry
			_picoModel.submitRenderables(renderer, localToWorld);
		}
	}
}

// Insert a light into this Instance's light list
void PicoModelInstance::insertLight(const RendererLight& light) {

	// Calculate transform from the superclass
	const Matrix4& localToWorld = Instance::localToWorld();
	
	// If the light's AABB intersects the oriented AABB of this model instance,
	// add the light to our light list
	if (light.testAABB(aabb_for_oriented_aabb(_picoModel.localAABB(),
											  localToWorld)))
	{
		_lights.addLight(light);
	}
}

} // namespace model
