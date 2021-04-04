#pragma once

#include "GLProgramAttributes.h"
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

/**
 * \brief
 * Renderable class containing a series of polygons textured with the same
 * material.
 *
 * StaticModelSurface objects are composited into a StaticModel object to
 * create a renderable static mesh.
 */
class StaticModelSurface :
	public IIndexedModelSurface,
	public OpenGLRenderable
{
private:
	// Name of the material this surface is using by default (without any skins)
	std::string _defaultMaterial;

	// Name of the material with skin remaps applied
	std::string _activeMaterial;

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
    // Move-construct this static model surface from the given vertex- and index array
	StaticModelSurface(std::vector<ArbitraryMeshVertex>&& vertices, std::vector<unsigned int>&& indices);

	// Copy-constructor.
	StaticModelSurface(const StaticModelSurface& other);

	~StaticModelSurface();

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
	void testSelect(Selector& selector, SelectionTest& test, 
        const Matrix4& localToWorld, bool twoSided) const;

	// IModelSurface implementation
	int getNumVertices() const override;
	int getNumTriangles() const override;

	const ArbitraryMeshVertex& getVertex(int vertexIndex) const override;
	ModelPolygon getPolygon(int polygonIndex) const override;

	const std::vector<ArbitraryMeshVertex>& getVertexArray() const override;
	const std::vector<unsigned int>& getIndexArray() const override;

	const std::string& getDefaultMaterial() const override;
	void setDefaultMaterial(const std::string& defaultMaterial);

	const std::string& getActiveMaterial() const override;
	void setActiveMaterial(const std::string& activeMaterial);

	// Returns true if the given ray intersects this surface geometry and fills in
	// the exact point in the given Vector3, returns false if no intersection was found.
	bool getIntersection(const Ray& ray, Vector3& intersection, const Matrix4& localToWorld);

	void applyScale(const Vector3& scale, const StaticModelSurface& originalSurface);
};
typedef std::shared_ptr<StaticModelSurface> StaticModelSurfacePtr;

}
