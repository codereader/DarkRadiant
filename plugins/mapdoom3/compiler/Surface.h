#pragma once

#include <vector>
#include <map>
#include "render/ArbitraryMeshVertex.h"
#include "math/AABB.h"
#include "math/Vector4.h"

namespace map
{

class Surface
{
public:
	struct SilEdge
	{
		// NOTE: making this a glIndex is dubious, as there can be 2x the faces as verts
		int	p1, p2;					// planes defining the edge
		int	v1, v2;					// verts defining the edge
	};

private:
	// typedefs needed to simulate the idHashIndex class
	typedef std::multimap<int, std::size_t> IndexLookupMap;
	typedef std::pair<typename IndexLookupMap::const_iterator, 
					  typename IndexLookupMap::const_iterator> Range;

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

	// set when the vertex tangents have been calculated
	bool		tangentsCalculated;

	// set when the face planes have been calculated
	bool		facePlanesCalculated;

	// mirroredVerts[0] is the mirror of vertices.size() - mirroredVerts.size() + 0
	std::vector<int> mirroredVerts;

	// pairs of the number of the first vertex and the number of the duplicate vertex
	std::vector<int> dupVerts;

	// this is used for calculating unsmoothed normals and tangents for deformed models
	struct DominantTri
	{
		int		v2, v3;
		float	normalizationScale[3];

		DominantTri() :
			v2(0),
			v3(0)
		{
			normalizationScale[0] = normalizationScale[1] = normalizationScale[2] = 0;
		}
	};

	std::vector<DominantTri> dominantTris;

	// [numIndexes/3] plane equations
	std::vector<Plane3> facePlanes;

	std::vector<Vector4> shadowVertices;			// these will be copied to shadowCache when it is going to be drawn.
													// these are NULL when vertex programs are available

	int shadowCapPlaneBits;
	std::size_t numShadowIndicesNoFrontCaps;	// shadow volumes with front caps omitted
	std::size_t numShadowIndicesNoCaps;			// shadow volumes with the front and rear caps omitted

	struct CullInfo 
	{
		// For each triangle a byte set to 1 if facing the light origin.
		std::vector<unsigned char> facing; // greebo: change to std::vector<bool>?

		// For each vertex a byte with the bits [0-5] set if the
		// vertex is at the back side of the corresponding clip plane.
		// If the 'cullBits' pointer equals LIGHT_CULL_ALL_FRONT all
		// vertices are at the front of all the clip planes.
		std::vector<unsigned char> cullBits;

		// Clip planes in surface space used to calculate the cull bits.
		Plane3		localClipPlanes[6];

		CullInfo()
		{
			localClipPlanes[0].dist() = localClipPlanes[1].dist() =
				localClipPlanes[2].dist() = localClipPlanes[3].dist() =
				localClipPlanes[4].dist() = localClipPlanes[5].dist() = 0;
		}
	};

	Surface() :
		perfectHull(false),
		tangentsCalculated(false),
		facePlanesCalculated(false),
		shadowCapPlaneBits(0),
		numShadowIndicesNoFrontCaps(0),
		numShadowIndicesNoCaps(0)
	{}

	void calcBounds();

	void cleanupTriangles(bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents);

	void cleanupUTriangles();

	// Writes the facePlanes values, overwriting existing ones if present
	void deriveFacePlanes();

	friend std::ostream& operator<<(std::ostream& str, const Surface& surface);

private:
	struct FaceTangents
	{
		Vector3 tangents[2];
		bool	negativePolarity;
		bool	degenerate;
	};

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

	// Modifies the surface to bust apart any verts that are shared by both positive and
	// negative texture polarities, so tangent space smoothing at the vertex doesn't
	// degenerate.
	// 
	// This will create some identical vertexes (which will eventually get different tangent
	// vectors), so never optimize the resulting mesh, or it will get the mirrored edges back.
	// 
	// Reallocates vertices and changes indices in place
	// Silindexes are unchanged by this.
	// 
	// sets mirroredVerts and mirroredVerts[]
	void duplicateMirroredVertexes();

	// Returns true if the texture polarity of the face is negative, false if it is positive or zero
	bool getFaceNegativePolarity(std::size_t firstIndex) const;

	void createDupVerts();

	// Find the largest triangle that uses each vertex
	void buildDominantTris();

	// Derives the normal and orthogonal tangent vectors for the triangle vertices.
	// For each vertex the normal and tangent vectors are derived from a single dominant triangle.
	void deriveUnsmoothedTangents();

	void deriveFaceTangents(std::vector<FaceTangents>& tangents);
	void deriveTangentsWithoutNormals();

	// Builds tangents, normals, and face planes
	void deriveTangents(bool allocFacePlanes);

	// Derives the normal and orthogonal tangent vectors for the triangle vertices.
	// For each vertex the normal and tangent vectors are derived from all triangles
	// using the vertex which results in smooth tangents across the mesh.
	// In the process the triangle planes are calculated as well.
	void deriveTangents(std::vector<Plane3>& planes);
};

} // namespace
