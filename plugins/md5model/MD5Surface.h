#ifndef MD5SURFACE_H_
#define MD5SURFACE_H_

#include "irender.h"
#include "render.h"
#include "renderable.h"
#include "math/aabb.h"
#include "math/frustum.h"
#include "cullable.h"
#include "selectable.h"

namespace md5
{

class MD5Surface : 
public OpenGLRenderable
{
public:
  typedef VertexBuffer<ArbitraryMeshVertex> vertices_t;
  typedef IndexBuffer indices_t;
private:

  AABB m_aabb_local;
  
	// Shader name
	std::string _shaderName;
	
	// Shader object
	ShaderPtr _shader;

  vertices_t m_vertices;
  indices_t m_indices;

private:

	// Capture the named shader
	void captureShader() {
		_shader = GlobalShaderCache().capture(_shaderName);
	}
	
public:

	// Constructor
	MD5Surface()
    : _shaderName("")
	{ }
	
  vertices_t& vertices()
  {
    return m_vertices;
  }
  indices_t& indices()
  {
    return m_indices;
  }

	// Set the shader name
	void setShader(const std::string& name) {
		_shaderName = name;
		captureShader();
	}

	/**
	 * Get the shader name.
	 */
	std::string getShader() const {
		return _shaderName;
	}
	
	/**
	 * Get the Shader object.
	 */
	ShaderPtr getState() const {
		return _shader;
	}
	
	// Refresh the AABB
	void updateAABB();

    // Back-end render function
    void render(RenderStateFlags state) const;

  VolumeIntersectionValue intersectVolume(const VolumeTest& test, const Matrix4& localToWorld) const
  {
    return test.TestAABB(m_aabb_local, localToWorld);
  }

  const AABB& localAABB() const
  {
    return m_aabb_local;
  }

  void render(Renderer& renderer, const Matrix4& localToWorld, ShaderPtr state) const
  {
    renderer.SetState(state, Renderer::eFullMaterials);
    renderer.addRenderable(*this, localToWorld);
  }

	void render(Renderer& renderer, const Matrix4& localToWorld) const {
		render(renderer, localToWorld, _shader);
	}

	// Test for selection
	void testSelect(Selector& selector, 
					SelectionTest& test, 
					const Matrix4& localToWorld);
};


}

#endif /*MD5SURFACE_H_*/
