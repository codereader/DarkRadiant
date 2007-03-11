#include "PicoModelInstance.h"

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
}

// Destructor
PicoModelInstance::~PicoModelInstance() {
    Instance::setTransformChangedCallback(Callback());
	GlobalShaderCache().detach(*this);
}

// Skin changed notify
void PicoModelInstance::skinChanged() {

	// Get the model skin object from the parent entity, and apply it to
	// the PicoModel if valid.
	ModelSkin* skin = NodeTypeCast<ModelSkin>::cast(path().parent());
	if (skin != NULL && skin->realised())
		_picoModel.applySkin(*skin);
	
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
	
		// Submit the model's geometry
		_picoModel.submitRenderables(renderer, localToWorld);
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
