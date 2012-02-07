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

	// indexes changed to be the first vertex with same XYZ, ignoring normal and texcoords
	Indices		silIndexes;

	void cleanupTriangles(bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents);

private:
	// Check for syntactically incorrect indexes, like out of range values.
	// Does not check for semantics, like degenerate triangles.
	// No vertexes is acceptable if no indexes.
	// No indexes is acceptable.
	// More vertexes than are referenced by indexes are acceptable.
	bool rangeCheckIndexes();

	// Uniquing vertexes only on xyz before creating sil edges reduces
	// the edge count by about 20% on Q3 models
	void createSilIndexes();

	std::vector<int> createSilRemap();

	void removeDegenerateTriangles();
};

} // namespace
