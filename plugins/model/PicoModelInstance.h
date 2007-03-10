#ifndef PICOMODELINSTANCE_H_
#define PICOMODELINSTANCE_H_

#include "RenderablePicoModel.h"

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
  public SkinnedModel
{
  class TypeCasts
  {
    InstanceTypeCastTable m_casts;
  public:
    TypeCasts()
    {
      InstanceContainedCast<PicoModelInstance, Bounded>::install(m_casts);
      InstanceContainedCast<PicoModelInstance, Cullable>::install(m_casts);
      InstanceStaticCast<PicoModelInstance, Renderable>::install(m_casts);
      InstanceStaticCast<PicoModelInstance, SelectionTestable>::install(m_casts);
      InstanceStaticCast<PicoModelInstance, SkinnedModel>::install(m_casts);
    }
    InstanceTypeCastTable& get()
    {
      return m_casts;
    }
  };

	// Reference to the actual model
	model::RenderablePicoModel& _picoModel;

	// The light list from the shader cache when we attach
	const LightList& _lightList;
	
public:

	typedef LazyStatic<TypeCasts> StaticTypeCasts;

	/* Main constructor */
	PicoModelInstance(const scene::Path& path, 
					  scene::Instance* parent, 
					  model::RenderablePicoModel& picomodel); 

	/* Destructor */	
	~PicoModelInstance();

	/**
	 * InstanceTypeCast to Bounded.
	 */
	Bounded& get(NullType<Bounded>) {
		return _picoModel;
	}
	
	/**
	 * InstanceTypeCast to Cullable.
	 */
	Cullable& get(NullType<Cullable>) {
		return _picoModel;
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
	void insertLight(const RendererLight& light) {
    	// Calculate transform and pass on to the pico model
    	const Matrix4& localToWorld = Instance::localToWorld();
		_picoModel.addLight(light, localToWorld);
	}
	
	// Clear all lights from this model instance
	void clearLights() {
		_picoModel.clearLights();
	}
};


}

#endif /*PICOMODELINSTANCE_H_*/
