#ifndef MD5SURFACE_H_
#define MD5SURFACE_H_

#include "irender.h"
#include "render.h"
#include "irenderable.h"
#include "math/aabb.h"
#include "math/frustum.h"
#include "cullable.h"
#include "selectable.h"
#include "modelskin.h"

namespace md5
{

class MD5Surface : 
public OpenGLRenderable
{
public:
	typedef VertexBuffer<ArbitraryMeshVertex> vertices_t;
	typedef IndexBuffer indices_t;
  
private:
	AABB _aabb_local;
  
	// Shader name
	std::string _shaderName;
	std::string _originalShaderName;
	
	// Shader object
	ShaderPtr _shader;

	vertices_t _vertices;
	indices_t _indices;

	// The GL display lists for this surface's geometry
	GLuint _normalList;
	GLuint _lightingList;

private:

	// Capture the named shader
	void captureShader();
	
	// Create the display lists
	void createDisplayLists();

public:

	/**
	 * Constructor.
	 */
	MD5Surface();
	
	/**
	 * Destructor.
	 */
	~MD5Surface();
	
	vertices_t& vertices();
	indices_t& indices();

	// Set/get the shader name
	void setShader(const std::string& name);
	std::string getShader() const;
	
	/**
	 * Get the Shader object.
	 */
	ShaderPtr getState() const;
	
	/**
	 * Calculate the AABB and build the display lists for rendering.
	 */ 
	void updateGeometry();

	// Applies the given Skin to this surface.
	void applySkin(const ModelSkin& skin);

    // Back-end render function
    void render(RenderStateFlags state) const;

	VolumeIntersectionValue intersectVolume(const VolumeTest& test, const Matrix4& localToWorld) const;

	const AABB& localAABB() const;

	void render(Renderer& renderer, const Matrix4& localToWorld, ShaderPtr state) const;
	void render(Renderer& renderer, const Matrix4& localToWorld) const;

	// Test for selection
	void testSelect(Selector& selector, 
					SelectionTest& test, 
					const Matrix4& localToWorld);
};
typedef boost::shared_ptr<MD5Surface> MD5SurfacePtr;

} // namespace md5

#endif /*MD5SURFACE_H_*/
