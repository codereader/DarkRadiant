#pragma once

#include <vector>
#include "render/ArbitraryMeshVertex.h"
#include "math/AABB.h"

namespace map
{

class Surface
{
public:
	AABB		bounds;

	typedef std::vector<ArbitraryMeshVertex> Vertices;
	Vertices	vertices;

	typedef std::vector<int> Indices;
	Indices		indices;

	void cleanupTriangles(bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents);

private:
	// Check for syntactically incorrect indexes, like out of range values.
	// Does not check for semantics, like degenerate triangles.
	// No vertexes is acceptable if no indexes.
	// No indexes is acceptable.
	// More vertexes than are referenced by indexes are acceptable.
	bool rangeCheckIndexes();
};

} // namespace
