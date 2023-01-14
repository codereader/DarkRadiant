#pragma once

// Math/Vertex classes
#include "render/MeshVertex.h"

class AABB;

namespace model
{

// A Polygon (Triangle) which is part of a model surface
struct ModelPolygon
{
	MeshVertex a;
	MeshVertex b;
	MeshVertex c;
};

// Abstract definition of a model surface
class IModelSurface
{
public:
	// Returns the number of vertices of this surface
	virtual int getNumVertices() const = 0;

	// Returns the number of tris of this surface
	virtual int getNumTriangles() const = 0;

	// Get a specific vertex of this surface
	virtual const MeshVertex& getVertex(int vertexNum) const = 0;

	/**
	 * greebo: Returns a specific polygon from this model surface.
	 * Don't expect this to be fast as things are returned by value.
	 * This is merely to provide read access to the model polygons
	 * for scripts and plugins.
	 */
	virtual ModelPolygon getPolygon(int polygonNum) const = 0;

	/**
	 * Get the name of the default material for this surface, i.e.
	 * the name of the material without any skin applied.
	 */
	virtual const std::string& getDefaultMaterial() const = 0;

	/**
	 * Return the name of the currently assigned material,
	 * respecting the applied skin.
	 */
	virtual const std::string& getActiveMaterial() const = 0;

    // Returns the local bounds of this surface
    virtual const AABB& getSurfaceBounds() const = 0;
};

/**
 * Model surface supporting direct access to its vertex and index arrays
 * which define the polygons. Model surfaces in DarkRadiant are loaded by
 * the PicoModel library which generates clockwise polygon windings.
 */
class IIndexedModelSurface :
	public IModelSurface
{
public:
	// Const access to the vertices used in this surface.
	virtual const std::vector<MeshVertex>& getVertexArray() const = 0;

	// Const access to the index array connecting the vertices.
	virtual const std::vector<unsigned int>& getIndexArray() const = 0;
};

} // namespace
