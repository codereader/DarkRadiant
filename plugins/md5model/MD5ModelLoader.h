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

#if !defined(INCLUDED_MODEL_H)
#define INCLUDED_MODEL_H

#include "MD5Surface.h"

#include "cullable.h"
#include "renderable.h"
#include "selectable.h"
#include "modelskin.h"
#include "nameable.h"

#include "math/frustum.h"
#include "string/string.h"
#include "generic/static.h"
#include "stream/stringstream.h"
#include "os/path.h"
#include "scenelib.h"
#include "instancelib.h"
#include "transformlib.h"
#include "traverselib.h"
#include "render.h"

class VectorLightList : public LightList
{
  typedef std::vector<const RendererLight*> Lights;
  Lights m_lights;
public:
  void addLight(const RendererLight& light)
  {
    m_lights.push_back(&light);
  }
  void clear()
  {
    m_lights.clear();
  }
  void evaluateLights() const
  {
  }
  void lightsChanged() const
  {
  }
  void forEachLight(const RendererLightCallback& callback) const
  {
    for(Lights::const_iterator i = m_lights.begin(); i != m_lights.end(); ++i)
    {
      callback(*(*i));
    }
  }
};

#include "MD5Model.h"

inline void Surface_addLight(const md5::MD5Surface& surface, VectorLightList& lights, const Matrix4& localToWorld, const RendererLight& light)
{
  if(light.testAABB(aabb_for_oriented_aabb(surface.localAABB(), localToWorld)))
  {
    lights.addLight(light);
  }
}

class ModelInstance :
  public scene::Instance,
  public Renderable,
  public SelectionTestable,
  public LightCullable,
  public SkinnedModel,
  public Bounded,
  public Cullable
{
  md5::MD5Model& m_model;

  const LightList* m_lightList;
  typedef Array<VectorLightList> SurfaceLightLists;
  SurfaceLightLists m_surfaceLightLists;

  class Remap
  {
  public:
    std::string first;
    ShaderPtr second;
  };
  
  typedef Array<Remap> SurfaceRemaps;
  SurfaceRemaps _surfaceRemaps;

  // The name of this model's skin
  std::string _skin;

public:
	// Bounded implementation
	virtual const AABB& localAABB() const {
		return m_model.localAABB();
	}
	
	// Cullable implementation
	virtual VolumeIntersectionValue intersectVolume(
		const VolumeTest& test, const Matrix4& localToWorld) const
	{
		return m_model.intersectVolume(test, localToWorld);
	}

  void lightsChanged()
  {
    m_lightList->lightsChanged();
  }
  typedef MemberCaller<ModelInstance, &ModelInstance::lightsChanged> LightsChangedCaller;

  void constructRemaps()
  {
	// greebo: Acquire the ModelSkin reference from the SkinCache
	// Note: This always returns a valid reference
	ModelSkin& skin = GlobalModelSkinCache().capture(_skin);

    SurfaceRemaps::iterator j = _surfaceRemaps.begin();
    for(md5::MD5Model::const_iterator i = m_model.begin(); i != m_model.end(); ++i, ++j)
    {
      std::string remap = skin.getRemap((*i)->getShader());
      if(!remap.empty())
      {
        j->first = remap;
        j->second = GlobalShaderCache().capture(remap);
      }
      else
      {
        j->second = ShaderPtr();
      }
    }
    SceneChangeNotify();
  }

  void destroyRemaps()
  {
    for(SurfaceRemaps::iterator i = _surfaceRemaps.begin(); i != _surfaceRemaps.end(); ++i)
    {
      if(i->second)
      {
        i->second = ShaderPtr();
      }
    }
  }

  // Returns the name of the currently active skin
  virtual std::string getSkin() const {
	  return _skin;
  }

  void skinChanged(const std::string& newSkinName)
  {
    ASSERT_MESSAGE(_surfaceRemaps.size() == m_model.size(), "ERROR");
    destroyRemaps();

	// greebo: Store the new skin name locally
	_skin = newSkinName;

    constructRemaps();
  }

  ModelInstance(const scene::Path& path, scene::Instance* parent, md5::MD5Model& model) :
    Instance(path, parent), 
    m_model(model),
    m_surfaceLightLists(m_model.size()),
    _surfaceRemaps(m_model.size())
  {
    m_lightList = &GlobalShaderCache().attach(*this);
    m_model._lightsChanged = LightsChangedCaller(*this);

    Instance::setTransformChangedCallback(LightsChangedCaller(*this));

    constructRemaps();
  }
  ~ModelInstance()
  {
    destroyRemaps();

    Instance::setTransformChangedCallback(Callback());

    m_model._lightsChanged = Callback();
    GlobalShaderCache().detach(*this);
  }

  void render(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const
  {
    SurfaceLightLists::const_iterator j = m_surfaceLightLists.begin();
    SurfaceRemaps::const_iterator k = _surfaceRemaps.begin();
    for(md5::MD5Model::const_iterator i = m_model.begin(); i != m_model.end(); ++i, ++j, ++k)
    {
      if((*i)->intersectVolume(volume, localToWorld) != c_volumeOutside)
      {
        renderer.setLights(*j);
        (*i)->render(renderer, localToWorld, (*k).second != 0 ? (*k).second : (*i)->getState());
      }
    }
  }

  void renderSolid(Renderer& renderer, const VolumeTest& volume) const
  {
    m_lightList->evaluateLights();

    render(renderer, volume, Instance::localToWorld());
  }
  void renderWireframe(Renderer& renderer, const VolumeTest& volume) const
  {
    renderSolid(renderer, volume);
  }

  void testSelect(Selector& selector, SelectionTest& test)
  {
    m_model.testSelect(selector, test, Instance::localToWorld());
  }

  bool testLight(const RendererLight& light) const
  {
    return light.testAABB(worldAABB());
  }
  void insertLight(const RendererLight& light)
  {
    const Matrix4& localToWorld = Instance::localToWorld();
    SurfaceLightLists::iterator j = m_surfaceLightLists.begin();
    for(md5::MD5Model::const_iterator i = m_model.begin(); i != m_model.end(); ++i)
    {
      Surface_addLight(*(*i), *j++, localToWorld, light);
    }
  }
  void clearLights()
  {
    for(SurfaceLightLists::iterator i = m_surfaceLightLists.begin(); i != m_surfaceLightLists.end(); ++i)
    {
      (*i).clear();
    }
  }
};

class ModelNode : 
	public scene::Node, 
	public scene::Instantiable,
	public Nameable
{
  InstanceSet m_instances;
  md5::MD5Model m_model;
public:
  ModelNode()
  {}

  md5::MD5Model& model()
  {
    return m_model;
  }

  scene::Instance* create(const scene::Path& path, scene::Instance* parent)
  {
    return new ModelInstance(path, parent, m_model);
  }
  void forEachInstance(const scene::Instantiable::Visitor& visitor)
  {
    m_instances.forEachInstance(visitor);
  }
  void insert(const scene::Path& path, scene::Instance* instance)
  {
    m_instances.insert(path, instance);
  }
  scene::Instance* erase(const scene::Path& path)
  {
    return m_instances.erase(path);
  }
  
  virtual std::string name() const {
  	return "MD5Model";
  }
};


inline void Surface_constructQuad(md5::MD5Surface& surface, const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d, const Vector3& normal)
{
  surface.vertices().push_back(
    ArbitraryMeshVertex(
      Vertex3f(a),
      Normal3f(normal),
      TexCoord2f(aabb_texcoord_topleft)
    )
  );
  surface.vertices().push_back(
    ArbitraryMeshVertex(
      Vertex3f(b),
      Normal3f(normal),
      TexCoord2f(aabb_texcoord_topright)
    )
  );
  surface.vertices().push_back(
    ArbitraryMeshVertex(
      Vertex3f(c),
      Normal3f(normal),
      TexCoord2f(aabb_texcoord_botright)
    )
  );
  surface.vertices().push_back(
    ArbitraryMeshVertex(
      Vertex3f(d),
      Normal3f(normal),
      TexCoord2f(aabb_texcoord_botleft)
    )
  );
}

inline void Model_constructNull(md5::MD5Model& model)
{
  md5::MD5Surface& surface = model.newSurface();

  AABB aabb(Vector3(0, 0, 0), Vector3(8, 8, 8));

  Vector3 points[8];
	aabb_corners(aabb, points);

  surface.vertices().reserve(24);

  Surface_constructQuad(surface, points[2], points[1], points[5], points[6], aabb_normals[0]);
  Surface_constructQuad(surface, points[1], points[0], points[4], points[5], aabb_normals[1]);
  Surface_constructQuad(surface, points[0], points[1], points[2], points[3], aabb_normals[2]);
  Surface_constructQuad(surface, points[0], points[3], points[7], points[4], aabb_normals[3]);
  Surface_constructQuad(surface, points[3], points[2], points[6], points[7], aabb_normals[4]);
  Surface_constructQuad(surface, points[7], points[6], points[5], points[4], aabb_normals[5]);

  surface.indices().reserve(36);

  RenderIndex indices[36] = {
     0,  1,  2,  0,  2,  3,
     4,  5,  6,  4,  6,  7,
     8,  9, 10,  8, 10, 11,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 21, 22, 10, 22, 23,
  };

  for(RenderIndex* i = indices; i != indices+(sizeof(indices)/sizeof(RenderIndex)); ++i)
  {
    surface.indices().insert(*i);
  }

  surface.setShader("");

  surface.updateGeometry();

  model.updateAABB();
}

#endif
