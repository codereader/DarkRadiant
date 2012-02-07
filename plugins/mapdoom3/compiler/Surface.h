#pragma once

#include <vector>
#include <map>
#include "render/ArbitraryMeshVertex.h"
#include "math/AABB.h"

namespace map
{

class Surface
{
private:
	// typedefs needed to simulate the idHashIndex class
	typedef std::multimap<int, std::size_t> IndexLookupMap;
	typedef std::pair<typename IndexLookupMap::const_iterator, 
					  typename IndexLookupMap::const_iterator> Range;

	struct SilEdge
	{
		// NOTE: making this a glIndex is dubious, as there can be 2x the faces as verts
		int	p1, p2;					// planes defining the edge
		int	v1, v2;					// verts defining the edge
	};

	IndexLookupMap _silEdgeLookup;

	static std::size_t MAX_SIL_EDGES;

	std::size_t _numDuplicatedEdges;
	std::size_t _numTripledEdges;
	std::size_t	_numPlanes;
	std::size_t _numSilEdges;

	static std::size_t _totalCoplanarSilEdges;
	static std::size_t _totalSilEdges;

public:
	AABB		bounds;

	typedef std::vector<ArbitraryMeshVertex> Vertices;
	Vertices	vertices;

	typedef std::vector<int> Indices;
	Indices		indices;

	// indexes changed to be the first vertex with same XYZ, ignoring normal and texcoords
	Indices		silIndexes;

	typedef std::vector<SilEdge> SilEdges;
	SilEdges	silEdges;

	// true if there aren't any dangling edges
	bool		perfectHull;

	Surface() :
		perfectHull(false)
	{}

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

	void testDegenerateTextureSpace();

	// If the surface will not deform, coplanar edges (polygon interiors)
	// can never create silhouette plains, and can be omited
	void identifySilEdges(bool omitCoplanarEdges);
	void defineEdge(int v1, int v2, int planeNum);
	static int SilEdgeSort(const void* a_, const void* b_);
};

} // namespace
