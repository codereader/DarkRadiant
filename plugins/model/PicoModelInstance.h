#ifndef PICOMODELINSTANCE_H_
#define PICOMODELINSTANCE_H_

#include "RenderablePicoModel.h"
#include "VectorLightList.h"

#include "math/frustum.h"
#include "scenelib.h"
#include "selectable.h"
#include "renderable.h"
#include "modelskin.h"
#include "irender.h"
#include "cullable.h"

#include <boost/utility.hpp>

namespace model
{

/**
 * Main Instance class for models.
 */
class PicoModelInstance :
  public boost::noncopyable,
  public scene::Instance,
  public Renderable,
  public SelectionTestable,
  public LightCullable,
  public SkinnedModel,
  public Bounded,
  public Cullable
{
	// Reference to the actual model
	RenderablePicoModel& _picoModel;

	// The light list from the shader cache when we attach
	const LightList& _lightList;

	// Vector of RendererLight references which illuminate this instance, set
	// with addLight() and clearLights()
	VectorLightList _lights;
	
	// Cache of RenderablePicoSurfaces along with their shaders. This is 
	// necessary to allow each Instance to have its own skin.
	typedef std::pair< boost::shared_ptr<RenderablePicoSurface>,
					   ShaderPtr> MappedSurface;
	typedef std::vector<MappedSurface> MappedSurfaces;
	MappedSurfaces _mappedSurfs;
	
public:
	/* Main constructor */
	PicoModelInstance(const scene::Path& path, 
					  scene::Instance* parent, 
					  model::RenderablePicoModel& picomodel); 

	/* Destructor */	
	~PicoModelInstance();

	// Bounded implementation
	virtual const AABB& localAABB() const {
		return _picoModel.localAABB();
	}
	
	// Cullable implementation
	virtual VolumeIntersectionValue intersectVolume(
		const VolumeTest& test, const Matrix4& localToWorld) const
	{
		return _picoModel.intersectVolume(test, localToWorld);
	}
	
	// Lights changed function
	void lightsChanged() {
		_lightList.lightsChanged();
	}
	
  	typedef MemberCaller<PicoModelInstance, &PicoModelInstance::lightsChanged> 
  	LightsChangedCaller;

	// Skin changed notify (from SkinnedModel)
	void skinChanged();
	
	/* Instance render function */
	void submitRenderables(Renderer& renderer, 
						   const VolumeTest& volume, 
						   const Matrix4& localToWorld) const;

	/* Required render functions */
	
  	void renderSolid(Renderer& renderer, const VolumeTest& volume) const {
    	_lightList.evaluateLights();

    	submitRenderables(renderer, volume, Instance::localToWorld());
	}
	
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
		renderSolid(renderer, volume);
	}

	// Test for a selection
	void testSelect(Selector& selector, SelectionTest& test) {
		_picoModel.testSelect(selector, test, Instance::localToWorld());
	}

	// LightCullable test function
	bool testLight(const RendererLight& light) const {
		return light.testAABB(worldAABB());
	}
	
	// Add a light to this model instance
	void insertLight(const RendererLight& light);
	
	// Clear all lights from this model instance
	void clearLights() {
		_lights.clear();
	}
};


}

#endif /*PICOMODELINSTANCE_H_*/
