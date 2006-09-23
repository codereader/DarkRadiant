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
	
	// Vector of ArbitraryMeshVertex structures, containing the coordinates,
	// normals, tangents and texture coordinates of the component vertices
	std::vector<ArbitraryMeshVertex> _vertices;
	
	// Vector of render indices, representing the groups of vertices to be
	// used to create triangles
	std::vector<unsigned int> _indices;
	
public:

	/** Constructor. Accepts a picoSurface_t struct and the file extension to determine
	 * how to assign materials.
	 */
	 
	RenderablePicoSurface(picoSurface_t* surf, const std::string& fExt);
	
	
	/** Render function from OpenGLRenderable
	 */
	 
	void render(RenderStateFlags flags) const {}
	
	
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
	
};

}

#endif /*RENDERABLEPICOSURFACE_H_*/
