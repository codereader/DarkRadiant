#ifndef RENDERABLEPICOSURFACE_H_
#define RENDERABLEPICOSURFACE_H_

#include "picomodel.h"
#include "irender.h"
#include "render.h"
#include "math/aabb.h"

/* FORWARD DECLS */

class ModelSkin; // modelskin.h

namespace model
{

/* Renderable class containing a series of polygons textured with the same
 * material. RenderablePicoSurface objects are composited into a RenderablePicoModel
 * object to create a renderable static mesh.
 */

class RenderablePicoSurface
: public OpenGLRenderable
{
	// Name of the material this surface is using, both originally and after a skin
	// remap.
	std::string _originalShaderName;
	std::string _mappedShaderName;
	
	// Shader object containing the material shader for this surface
	Shader* _shader;
	
	// Vector of ArbitraryMeshVertex structures, containing the coordinates,
	// normals, tangents and texture coordinates of the component vertices
	std::vector<ArbitraryMeshVertex> _vertices;
	
	// Vector of render indices, representing the groups of vertices to be
	// used to create triangles
	std::vector<unsigned int> _indices;
	
	// Keep track of the number of indices to iterate over, since vector::size()
	// may not be fast
	unsigned int _nIndices;

	// The AABB containing this surface, in local object space.
	AABB _localAABB;
	
public:

	/** Constructor. Accepts a picoSurface_t struct and the file extension to determine
	 * how to assign materials.
	 */
	RenderablePicoSurface(picoSurface_t* surf, const std::string& fExt);
	
	/** Destructor. Vectors will be automatically destructed, but we need to release the
	 * shader.
	 */
	~RenderablePicoSurface() {
		GlobalShaderCache().release(_mappedShaderName);
	}
	
	/** Render function from OpenGLRenderable
	 */
	void render(RenderStateFlags flags) const;
	
	/** Return the vertex count for this surface
	 */
	int getVertexCount() const {
		return _vertices.size();
	}
	
	/** Return the poly count for this surface
	 */
	int getPolyCount() const {
		return _indices.size() / 3; // 3 indices per triangle
	}
	
	/** Get the Shader for this surface.
	 */
	Shader* getShader() const {
		return _shader;
	}	 

	/** Get the containing AABB for this surface.
	 */
	const AABB& getAABB() const {
		return _localAABB;	
	}
	
	/** Apply the provided skin to this surface. If the skin has a remap for
	 * this surface's material, it will be applied, otherwise no action will
	 * occur.
	 * 
	 * @param skin
	 * ModelSkin object to apply to this surface.
	 */
	void applySkin(const ModelSkin& skin);
	 
};

}

#endif /*RENDERABLEPICOSURFACE_H_*/
