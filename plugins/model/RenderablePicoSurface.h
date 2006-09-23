#ifndef RENDERABLEPICOSURFACE_H_
#define RENDERABLEPICOSURFACE_H_

#include "picomodel.h"
#include "irender.h"
#include "render.h"

namespace model
{

/* Renderable class containing a series of polygons textured with the same
 * material. RenderablePicoSurface objects are composited into a RenderablePicoModel
 * object to create a renderable static mesh.
 */

class RenderablePicoSurface
: public OpenGLRenderable
{
	// Name of the material this surface is using
	std::string _shaderName;
	
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
	
public:

	/** Constructor. Accepts a picoSurface_t struct and the file extension to determine
	 * how to assign materials.
	 */
	RenderablePicoSurface(picoSurface_t* surf, const std::string& fExt);
	
	/** Destructor. Vectors will be automatically destructed, but we need to release the
	 * shader.
	 */
	~RenderablePicoSurface() {
		GlobalShaderCache().release(_shaderName);
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

	
};

}

#endif /*RENDERABLEPICOSURFACE_H_*/
