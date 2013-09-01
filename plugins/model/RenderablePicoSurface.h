#pragma once

#include "GLProgramAttributes.h"
#include "picomodel.h"
#include "render.h"
#include "math/AABB.h"

#include "ishaders.h"
#include "imodelsurface.h"

/* FORWARD DECLS */
class ModelSkin;
class RenderableCollector;
class SelectionTest;
class Selector;
class Shader;
class Ray;

namespace model
{

/* Renderable class containing a series of polygons textured with the same
 * material. RenderablePicoSurface objects are composited into a RenderablePicoModel
 * object to create a renderable static mesh.
 */

class RenderablePicoSurface :
	public IModelSurface,
	public OpenGLRenderable
{
	// Name of the material this surface is using
	std::string _shaderName;

	// Vector of ArbitraryMeshVertex structures, containing the coordinates,
	// normals, tangents and texture coordinates of the component vertices
	typedef std::vector<ArbitraryMeshVertex> VertexVector;
	VertexVector _vertices;

	// Vector of render indices, representing the groups of vertices to be
	// used to create triangles
	typedef std::vector<unsigned int> Indices;
	Indices _indices;

	// Keep track of the number of indices to iterate over, since vector::size()
	// may not be fast
	unsigned int _nIndices;

	// The AABB containing this surface, in local object space.
	AABB _localAABB;

	// The GL display lists for this surface's geometry
	GLuint _dlRegular;
	GLuint _dlProgramVcol;
    GLuint _dlProgramNoVCol;

private:

	// Get a colour vector from an unsigned char array (may be NULL)
	Vector3 getColourVector(unsigned char* array);

	// Calculate tangent and bitangent vectors for all vertices.
	void calculateTangents();

	// Create the display lists
    GLuint compileProgramList(bool includeColour);
	void createDisplayLists();

	std::string cleanupShaderName(const std::string& mapName);

public:
	/**
	 * Constructor. Accepts a picoSurface_t struct and the file extension to determine
	 * how to assign materials.
	 */
	RenderablePicoSurface(picoSurface_t* surf, const std::string& fExt);

	/**
	 * Destructor.
	 */
	~RenderablePicoSurface();

	/**
	 * Front-end render function used by the main renderer.
	 *
	 * @param rend
	 * The sorting RenderableCollector object which accepts renderable geometry.
	 *
	 * @param localToWorld
	 * Object to world-space transform.
	 *
	 * @param shader
	 * The shader to submit ourselves as renderable
	 *
	 * @param entity
	 * The entity this object is attached to.
	 */
	void submitRenderables(RenderableCollector& rend, const Matrix4& localToWorld,
						   const ShaderPtr& shader, const IRenderEntity& entity);

	void setRenderSystem(const RenderSystemPtr& renderSystem);

	/**
	 * Render function from OpenGLRenderable
	 */
	void render(const RenderInfo& info) const;

	/** Get the containing AABB for this surface.
	 */
	const AABB& getAABB() const {
		return _localAABB;
	}

	/**
	 * Perform a selection test on this surface.
	 */
	void testSelect(Selector& selector,
					SelectionTest& test,
					const Matrix4& localToWorld) const;

	// IModelSurface implementation
	int getNumVertices() const;
	int getNumTriangles() const;

	const ArbitraryMeshVertex& getVertex(int vertexIndex) const;
	ModelPolygon getPolygon(int polygonIndex) const;

	const std::string& getDefaultMaterial() const;
	void setDefaultMaterial(const std::string& defaultMaterial);

	// Returns true if the given ray intersects this surface geometry and fills in
	// the exact point in the given Vector3, returns false if no intersection was found.
	bool getIntersection(const Ray& ray, Vector3& intersection, const Matrix4& localToWorld);

private:
	void captureShader();
};
typedef boost::shared_ptr<RenderablePicoSurface> RenderablePicoSurfacePtr;

}
