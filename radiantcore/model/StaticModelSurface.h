#pragma once

#include "imodelsurface.h"
#include "ishaders.h"

#include "math/AABB.h"

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
 * A static surface contains a series of triangles textured with the same
 * material. The number of vertices and indices of a surface never changes.
 *
 * StaticModelSurface objects are composited into a StaticModel object to
 * create a renderable static mesh.
 */
class StaticModelSurface final :
	public IIndexedModelSurface
{
private:
	// Name of the material this surface is using by default (without any skins)
	std::string _defaultMaterial;

	// Name of the material with skin remaps applied
	std::string _activeMaterial;

	// Vector of MeshVertex structures, containing the coordinates,
	// normals, tangents and texture coordinates of the component vertices
	typedef std::vector<MeshVertex> VertexVector;
	VertexVector _vertices;

	// Vector of render indices, representing the groups of vertices to be
	// used to create triangles
	typedef std::vector<unsigned int> Indices;
	Indices _indices;

	// The AABB containing this surface, in local object space.
	AABB _localAABB;

private:
	// Calculate tangent and bitangent vectors for all vertices.
	void calculateTangents();

public:
    // Move-construct this static model surface from the given vertex- and index array
	StaticModelSurface(std::vector<MeshVertex>&& vertices, std::vector<unsigned int>&& indices);

	// Copy-constructor. All vertices and indices will be copied from 'other'.
	StaticModelSurface(const StaticModelSurface& other);

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

	const MeshVertex& getVertex(int vertexIndex) const override;
	ModelPolygon getPolygon(int polygonIndex) const override;

	const std::vector<MeshVertex>& getVertexArray() const override;
	const std::vector<unsigned int>& getIndexArray() const override;

	const std::string& getDefaultMaterial() const override;
	void setDefaultMaterial(const std::string& defaultMaterial);

	const std::string& getActiveMaterial() const override;
	void setActiveMaterial(const std::string& activeMaterial);

    const AABB& getSurfaceBounds() const override;

	// Returns true if the given ray intersects this surface geometry and fills in
	// the exact point in the given Vector3, returns false if no intersection was found.
	bool getIntersection(const Ray& ray, Vector3& intersection, const Matrix4& localToWorld);

	void applyScale(const Vector3& scale, const StaticModelSurface& originalSurface);
};
typedef std::shared_ptr<StaticModelSurface> StaticModelSurfacePtr;

}
