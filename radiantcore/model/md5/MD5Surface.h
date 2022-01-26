#pragma once

#include "irender.h"
#include "render.h"
#include "math/AABB.h"
#include "math/Frustum.h"
#include "iselectiontest.h"
#include "modelskin.h"
#include "imodelsurface.h"

#include "MD5DataStructures.h"
#include "parser/DefTokeniser.h"

class Ray;

namespace md5
{

class MD5Skeleton;

class MD5Surface final :
	public model::IIndexedModelSurface
{
public:
	typedef std::vector<ArbitraryMeshVertex> Vertices;
	typedef IndexBuffer Indices;

private:
	AABB _aabb_local;

	// Default shader name
	std::string _originalShaderName;

	// The name of the currently active material (afer skin application)
	std::string _activeMaterial;

	// The mesh definition - will be baked into renderable vertex arrays
	// Several MD5Surfaces can share the same mesh
	MD5MeshPtr _mesh;

	// Our render data
	Vertices _vertices;
	Indices _indices;

public:

	MD5Surface();

	// Copy constructor, re-uses the MD5Mesh of <other>.
	MD5Surface(const MD5Surface& other);

	// Set/get the shader name
	void setDefaultMaterial(const std::string& name);
	
	/**
	 * Calculate the AABB and build the display lists for rendering.
	 */
	void updateGeometry();

	// Updates the mesh to the pose defined in the .md5mesh file - usually a T-Pose
	// It needs the joints defined in that file as reference
	void updateToDefaultPose(const MD5Joints& joints);

	// Updates this mesh to the state of the given skeleton
	void updateToSkeleton(const MD5Skeleton& skeleton);

	const AABB& localAABB() const;

	// Test for selection
	void testSelect(Selector& selector,
					SelectionTest& test,
					const Matrix4& localToWorld);

	bool getIntersection(const Ray& ray, Vector3& intersection, const Matrix4& localToWorld);

	// IModelSurface implementation
	int getNumVertices() const override;
	int getNumTriangles() const override;

	const ArbitraryMeshVertex& getVertex(int vertexIndex) const override;
	model::ModelPolygon getPolygon(int polygonIndex) const override;

	const std::vector<ArbitraryMeshVertex>& getVertexArray() const override;
	const std::vector<unsigned int>& getIndexArray() const override;

	const std::string& getDefaultMaterial() const override;
	const std::string& getActiveMaterial() const override;
	void setActiveMaterial(const std::string& activeMaterial);

    const AABB& getSurfaceBounds() const override;

	void parseFromTokens(parser::DefTokeniser& tok);

	// Rebuild the render index array - usually needs to be called only once
	void buildIndexArray();

private:
    // Re-calculate the normal vectors
    void buildVertexNormals();
};
typedef std::shared_ptr<MD5Surface> MD5SurfacePtr;

} // namespace
