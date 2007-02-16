/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "model.h"

#include "picomodel.h"
#include "RenderablePicoModel.h"

#include "iarchive.h"
#include "idatastream.h"
#include "imodel.h"
#include "modelskin.h"

#include "cullable.h"
#include "renderable.h"
#include "selectable.h"

#include "math/frustum.h"
#include "generic/static.h"
#include "shaderlib.h"
#include "scenelib.h"
#include "instancelib.h"
#include "transformlib.h"
#include "traverselib.h"
#include "render.h"

#include "os/path.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/utility.hpp>

#include <string>

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

  void* m_test;

  Bounded& get(NullType<Bounded>)
  {
    return _picoModel;
  }
  Cullable& get(NullType<Cullable>)
  {
    return _picoModel;
  }

	// Lights changed function
	void lightsChanged() {
		_lightList.lightsChanged();
	}
	
  	typedef MemberCaller<PicoModelInstance, &PicoModelInstance::lightsChanged> 
  	LightsChangedCaller;

	// Skin changed notify (from SkinnedModel)
	void skinChanged() {

	}

	/* Main constructor */
	PicoModelInstance(const scene::Path& path, 
					  scene::Instance* parent, 
					  model::RenderablePicoModel& picomodel) 
	: Instance(path, parent, this, StaticTypeCasts::instance().get()),
      _picoModel(picomodel),
	  _lightList(GlobalShaderCache().attach(*this))
	{
		Instance::setTransformChangedCallback(LightsChangedCaller(*this));
	}

	/* Destructor */	
	~PicoModelInstance() {
	    Instance::setTransformChangedCallback(Callback());

		GlobalShaderCache().detach(*this);
	}

	/* Instance render function */
	void submitRenderables(Renderer& renderer, 
						   const VolumeTest& volume, 
						   const Matrix4& localToWorld) const
	{
		// Test the model's intersection volume, if it intersects pass on the 
		// render call
		if (_picoModel.intersectVolume(volume, localToWorld) 
			!= c_volumeOutside)
		{
			_picoModel.submitRenderables(renderer, localToWorld);
		}
	}

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

class PicoModelNode : public scene::Node::Symbiot, public scene::Instantiable
{
  class TypeCasts
  {
    NodeTypeCastTable m_casts;
  public:
    TypeCasts()
    {
      NodeStaticCast<PicoModelNode, scene::Instantiable>::install(m_casts);
    }
    NodeTypeCastTable& get()
    {
      return m_casts;
    }
  };


  scene::Node m_node;
  InstanceSet m_instances;

	// The actual model
	model::RenderablePicoModel _picoModel;

public:
  typedef LazyStatic<TypeCasts> StaticTypeCasts;

  /** Construct a PicoModelNode with the parsed picoModel_t struct and the
   * provided file extension.
   */
  PicoModelNode(picoModel_t* model, const std::string& ext)
  : m_node(this, this, StaticTypeCasts::instance().get()), 
    _picoModel(model, ext) // pass extension down to the PicoModel
  {
  }

  void release()
  {
    delete this;
  }
  scene::Node& node()
  {
    return m_node;
  }

  scene::Instance* create(const scene::Path& path, scene::Instance* parent)
  {
    return new PicoModelInstance(path, parent, _picoModel);
  }
  void forEachInstance(const scene::Instantiable::Visitor& visitor)
  {
    m_instances.forEachInstance(visitor);
  }
  void insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance)
  {
    m_instances.insert(observer, path, instance);
  }
  scene::Instance* erase(scene::Instantiable::Observer* observer, const scene::Path& path)
  {
    return m_instances.erase(observer, path);
  }
};


size_t picoInputStreamReam(void* inputStream, unsigned char* buffer, size_t length)
{
  return reinterpret_cast<InputStream*>(inputStream)->read(buffer, length);
}

/* Use the picomodel library to load the contents of the given file 
 * and return a Node containing the model.
 */

scene::Node& loadPicoModel(const picoModule_t* module, ArchiveFile& file) {

	// Determine the file extension (ASE or LWO) to pass down to the PicoModel
	std::string fName = file.getName();
	boost::algorithm::to_lower(fName);
	std::string fExt = fName.substr(fName.size() - 3, 3);

	picoModel_t* model = PicoModuleLoadModelStream(module, &file.getInputStream(), picoInputStreamReam, file.size(), 0);
	PicoModelNode* modelNode = new PicoModelNode(model, fExt);
	PicoFreeModel(model);
	return modelNode->node();
}

/* Load the provided file as a model object and return as an IModel
 * shared pointer.
 */
 
#include "RenderablePicoModel.h"
 
model::IModelPtr loadIModel(const picoModule_t* module, ArchiveFile& file) {

	// Determine the file extension (ASE or LWO) to pass down to the PicoModel
	std::string fName = file.getName();
	boost::algorithm::to_lower(fName);
	std::string fExt = fName.substr(fName.size() - 3, 3);

	picoModel_t* model = PicoModuleLoadModelStream(module, &file.getInputStream(), picoInputStreamReam, file.size(), 0);
	
	model::IModelPtr modelObj(new model::RenderablePicoModel(model, fExt));
	PicoFreeModel(model);
	return modelObj;
	
} 
