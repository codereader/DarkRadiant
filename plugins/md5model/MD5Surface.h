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
	
	// Release the named shader
	void releaseShader() {
		_shader = ShaderPtr();
	}

public:

	// Constructor
	MD5Surface()
    : _shaderName("")
	{ }
	
	// Destructor. Release the shader
	~MD5Surface() {
		releaseShader();
	}

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
		releaseShader();
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
	
  void updateAABB()
  {
    m_aabb_local = AABB();
    for(vertices_t::iterator i = m_vertices.begin(); i != m_vertices.end(); ++i)
      m_aabb_local.includePoint(reinterpret_cast<const Vector3&>(i->vertex));



    for(MD5Surface::indices_t::iterator i = m_indices.begin(); i != m_indices.end(); i += 3)
    {
			ArbitraryMeshVertex& a = m_vertices[*(i + 0)];
			ArbitraryMeshVertex& b = m_vertices[*(i + 1)];
			ArbitraryMeshVertex& c = m_vertices[*(i + 2)];

      ArbitraryMeshTriangle_sumTangents(a, b, c);
    }

    for(MD5Surface::vertices_t::iterator i = m_vertices.begin(); i != m_vertices.end(); ++i)
    {
      vector3_normalise(reinterpret_cast<Vector3&>((*i).tangent));
      vector3_normalise(reinterpret_cast<Vector3&>((*i).bitangent));
    }
  }

  void render(RenderStateFlags state) const
  {
    if((state & RENDER_BUMP) != 0)
    {
      /*if(GlobalShaderCache().useShaderLanguage())
      {
        glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_vertices.data()->normal);
        glVertexAttribPointerARB(c_attr_TexCoord0, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_vertices.data()->texcoord);
        glVertexAttribPointerARB(c_attr_Tangent, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_vertices.data()->tangent);
        glVertexAttribPointerARB(c_attr_Binormal, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_vertices.data()->bitangent);
      }
      else
      {*/
        glVertexAttribPointerARB(11, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_vertices.data()->normal);
        glVertexAttribPointerARB(8, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_vertices.data()->texcoord);
        glVertexAttribPointerARB(9, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_vertices.data()->tangent);
        glVertexAttribPointerARB(10, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &m_vertices.data()->bitangent);
      /*}*/
    }
    else
    {
      glNormalPointer(GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_vertices.data()->normal);
      glTexCoordPointer(2, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_vertices.data()->texcoord);
    }
    glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &m_vertices.data()->vertex);
    glDrawElements(GL_TRIANGLES, GLsizei(m_indices.size()), RenderIndexTypeID, m_indices.data());

  }

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
