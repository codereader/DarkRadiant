#pragma once

// Math/Vertex classes
#include "render/ArbitraryMeshVertex.h"

namespace model
{

// A Polygon (Triangle) which is part of a model surface
struct ModelPolygon
{
	ArbitraryMeshVertex a;
	ArbitraryMeshVertex b;
	ArbitraryMeshVertex c;
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
	virtual const ArbitraryMeshVertex& getVertex(int vertexIndex) const = 0;

	/**
	 * greebo: Returns a specific polygon from this model surface.
	 * Don't expect this to be fast as things are returned by value.
	 * This is merely to provide read access to the model polygons
	 * for scripts and plugins.
	 */
	virtual ModelPolygon getPolygon(int polygonIndex) const = 0;

	/**
	 * Get the name of the default material for this surface, i.e.
	 * the name of the material without any skin applied.
	 */
	virtual const std::string& getDefaultMaterial() const = 0;
};

} // namespace
