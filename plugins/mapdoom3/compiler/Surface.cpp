#include "Surface.h"

#include <map>
#include "itextstream.h"
#include "render/ArbitraryMeshVertex.h"

namespace map
{

std::size_t Surface::MAX_SIL_EDGES = 0x10000;
std::size_t	Surface::_totalCoplanarSilEdges = 0;
std::size_t Surface::_totalSilEdges = 0;

void Surface::calcBounds()
{
	bounds == AABB();

	for (Vertices::const_iterator i = vertices.begin(); i != vertices.end(); ++i)
	{
		bounds.includePoint(i->vertex);
	}
}

bool Surface::rangeCheckIndexes()
{
	if (indices.empty())
	{
		globalErrorStream() << "Surface::rangeCheckIndexes: no indices" << std::endl;
		return false;
	}

	if (vertices.empty())
	{
		globalErrorStream() << "Surface::rangeCheckIndexes: no vertices" << std::endl;
		return false;
	}

	// must specify an integral number of triangles
	if (indices.size() % 3 != 0 )
	{
		globalErrorStream() << "Surface::rangeCheckIndexes: indices mod 3" << std::endl;
		return false;
	}

	for (std::size_t i = 0; i < indices.size(); ++i)
	{
		if (indices[i] < 0 || indices[i] >= vertices.size())
		{
			globalErrorStream() << "Surface::rangeCheckIndexes: index out of range" << std::endl;
			return false;
		}
	}

	// this should not be possible unless there are unused verts
	//if ( tri->numVerts > tri->numIndexes ) {
		// FIXME: find the causes of these
		// common->Printf( "R_RangeCheckIndexes: tri->numVerts > tri->numIndexes\n" );
	//}

	return true;
}

std::vector<int> Surface::createSilRemap()
{
	std::vector<int> remap(vertices.size());

	static int hashSize = 1024;

	if (false/* !r_useSilRemap.GetBool() */)
	{
		for (std::size_t i = 0 ; i < vertices.size(); ++i)
		{
			remap[i] = static_cast<int>(i);
		}

		return remap;
	}

	IndexLookupMap lookup;

	std::size_t removed = 0;
	std::size_t unique = 0;

	for (std::size_t i = 0 ; i < vertices.size(); ++i)
	{
		const ArbitraryMeshVertex& v1 = vertices[i];

		// see if there is an earlier vert that it can map to
		int	hashKey = (static_cast<int>(v1.vertex[0]) + 
					   static_cast<int>(v1.vertex[1]) + 
					   static_cast<int>(v1.vertex[2])) & hashSize;
		
		Range range = lookup.equal_range(hashKey);

		IndexLookupMap::const_iterator j;

		for (j = range.first; j != range.second; ++j)
		{
			const ArbitraryMeshVertex& v2 = vertices[j->second];

			if (v2.vertex == v1.vertex)
			{
				removed++;
				remap[i] = static_cast<int>(j->second);
				break;
			}
		}

		if (j == range.second)
		{
			unique++;
			remap[i] = static_cast<int>(i);

			lookup.insert(IndexLookupMap::value_type(hashKey, i));
		}
	}

	return remap;
}

void Surface::createSilIndexes()
{
	silIndexes.clear();

	std::vector<int> remap = createSilRemap();

	// remap indexes to the first one
	silIndexes.resize(indices.size());

	for (std::size_t i = 0; i < indices.size(); ++i)
	{
		silIndexes[i] = remap[indices[i]];
	}
}

void Surface::removeDegenerateTriangles()
{
	std::size_t numRemoved = 0;

	// check for completely degenerate triangles
	for (std::size_t i = 0; i < indices.size(); )
	{
		int a = silIndexes[i];
		int b = silIndexes[i+1];
		int c = silIndexes[i+2];

		if (a == b || a == c || b == c)
		{
			numRemoved++;

			// remove three indices
			indices.erase(indices.begin() + i, indices.begin() + i + 2);

			if (!silIndexes.empty())
			{
				silIndexes.erase(silIndexes.begin() + i, silIndexes.begin() + i + 2);
			}

			// Size of vector is decreased by 3 now, no need to increase i
		}
		else
		{
			i += 3;
		}
	}

	// this doesn't free the memory used by the unused verts

	if (numRemoved > 0)
	{
		globalOutputStream() << (boost::format("removed %i degenerate triangles") % numRemoved) << std::endl;
	}
}

void Surface::testDegenerateTextureSpace()
{
	// check for triangles with a degenerate texture space
	std::size_t numDegenerate = 0;

	for (std::size_t i = 0; i < indices.size(); i += 3 )
	{
		const ArbitraryMeshVertex& a = vertices[indices[i+0]];
		const ArbitraryMeshVertex& b = vertices[indices[i+1]];
		const ArbitraryMeshVertex& c = vertices[indices[i+2]];

		if (a.texcoord == b.texcoord || b.texcoord == c.texcoord || c.texcoord == a.texcoord)
		{
			numDegenerate++;
		}
	}

	if (numDegenerate > 0)
	{
//		globalOutputStream() << (boost::format("%d triangles with a degenerate texture space") % numDegenerate) << std::endl;
	}
}

void Surface::defineEdge(int v1, int v2, int planeNum)
{
	// check for degenerate edge
	if (v1 == v2)
	{
		return;
	}

	static int SIL_EDGE_HASHSIZE = 1024;

	int hashKey = (v1 + v1) & SIL_EDGE_HASHSIZE;

	// search for a matching other side
	Range range = _silEdgeLookup.equal_range(hashKey);

	IndexLookupMap::const_iterator j;

	for (j = range.first; j != range.second; ++j)
	{
		std::size_t index = j->second;

		if (silEdges[index].v1 == v1 && silEdges[index].v2 == v2)
		{
			_numDuplicatedEdges++;
			// allow it to still create a new edge
			continue;
		}

		if (silEdges[index].v2 == v1 && silEdges[index].v1 == v2)
		{
			if (silEdges[index].p2 != _numPlanes)
			{
				_numTripledEdges++;
				// allow it to still create a new edge
				continue;
			}

			// this is a matching back side
			silEdges[index].p2 = planeNum;
			return;
		}
	}

	// define the new edge
	if (_numSilEdges == MAX_SIL_EDGES)
	{
		globalWarningStream() << "MAX_SIL_EDGES" << std::endl;
		return;
	}
	
	_silEdgeLookup.insert(IndexLookupMap::value_type(hashKey, _numSilEdges));

	silEdges[_numSilEdges].p1 = planeNum;
	silEdges[_numSilEdges].p2 = static_cast<int>(_numPlanes);
	silEdges[_numSilEdges].v1 = v1;
	silEdges[_numSilEdges].v2 = v2;

	_numSilEdges++;
}

int Surface::SilEdgeSort(const void* a_, const void* b_)
{
	const SilEdge* a = reinterpret_cast<const SilEdge*>(a_);
	const SilEdge* b = reinterpret_cast<const SilEdge*>(b_);

	if (a->p1 < b->p1)
	{
		return -1;
	}

	if (a->p1 > b->p1)
	{
		return 1;
	}

	if (a->p2 < b->p2)
	{
		return -1;
	}

	if (a->p2 > b->p2)
	{
		return 1;
	}

	return 0;
}

void Surface::identifySilEdges(bool omitCoplanarEdges)
{
	omitCoplanarEdges = false;	// optimization doesn't work for some reason

	std::size_t numTris = indices.size() / 3;

	_numSilEdges = 0;
	_silEdgeLookup.clear();
	silEdges.resize(MAX_SIL_EDGES);

	_numPlanes = numTris;

	_numDuplicatedEdges = 0;
	_numTripledEdges = 0;

	for (std::size_t i = 0 ; i < numTris; ++i)
	{
		int i1 = silIndexes[i*3 + 0];
		int i2 = silIndexes[i*3 + 1];
		int i3 = silIndexes[i*3 + 2];

		// create the edges
		defineEdge(i1, i2, static_cast<int>(i));
		defineEdge(i2, i3, static_cast<int>(i));
		defineEdge(i3, i1, static_cast<int>(i));
	}

	if (_numDuplicatedEdges > 0 || _numTripledEdges > 0)
	{
		globalWarningStream() << (boost::format("%i duplicated edge directions, %i tripled edges") % 
			_numDuplicatedEdges % _numTripledEdges) << std::endl;
	}

	// if we know that the vertexes aren't going
	// to deform, we can remove interior triangulation edges
	// on otherwise planar polygons.
	// I earlier believed that I could also remove concave
	// edges, because they are never silhouettes in the conventional sense,
	// but they are still needed to balance out all the true sil edges
	// for the shadow algorithm to function
	std::size_t	_numCoplanarCulled = 0;

	if (omitCoplanarEdges)
	{
#if 0
		for ( i = 0 ; i < numSilEdges ; i++ ) {
			int			i1, i2, i3;
			idPlane		plane;
			int			base;
			int			j;
			float		d;

			if ( silEdges[i].p2 == numPlanes ) {	// the fake dangling edge
				continue;
			}

			base = silEdges[i].p1 * 3;
			i1 = tri->silIndexes[ base + 0 ];
			i2 = tri->silIndexes[ base + 1 ];
			i3 = tri->silIndexes[ base + 2 ];

			plane.FromPoints( tri->verts[i1].xyz, tri->verts[i2].xyz, tri->verts[i3].xyz );

			// check to see if points of second triangle are not coplanar
			base = silEdges[i].p2 * 3;
			for ( j = 0 ; j < 3 ; j++ ) {
				i1 = tri->silIndexes[ base + j ];
				d = plane.Distance( tri->verts[i1].xyz );
				if ( d != 0 ) {		// even a small epsilon causes problems
					break;
				}
			}

			if ( j == 3 ) {
				// we can cull this sil edge
				memmove( &silEdges[i], &silEdges[i+1], (numSilEdges-i-1) * sizeof( silEdges[i] ) );
				c_coplanarCulled++;
				numSilEdges--;
				i--;
			}
		}
		if ( c_coplanarCulled ) {
			c_coplanarSilEdges += c_coplanarCulled;
//			common->Printf( "%i of %i sil edges coplanar culled\n", c_coplanarCulled,
//				c_coplanarCulled + numSilEdges );
		}
#endif
	}
	_totalSilEdges += _numSilEdges;

	// sort the sil edges based on plane number
	qsort(&(silEdges[0]), _numSilEdges, sizeof(silEdges[0]), SilEdgeSort);

	// count up the distribution.
	// a perfectly built model should only have shared
	// edges, but most models will have some interpenetration
	// and dangling edges
	std::size_t shared = 0;
	std::size_t single = 0;

	for (std::size_t i = 0; i < _numSilEdges; ++i)
	{
		if (silEdges[i].p2 == _numPlanes)
		{
			single++;
		}
		else
		{
			shared++;
		}
	}

	perfectHull = (single == 0);

	// Condense the vector to actual size
	silEdges.resize(_numSilEdges);
}

namespace
{
	struct TangentVertex 
	{
		bool	polarityUsed[2];
		int		negativeRemap;
	};
}

bool Surface::getFaceNegativePolarity(std::size_t firstIndex) const
{
	float d0[5], d1[5];

	const ArbitraryMeshVertex& a = vertices[indices[firstIndex + 0]];
	const ArbitraryMeshVertex& b = vertices[indices[firstIndex + 1]];
	const ArbitraryMeshVertex& c = vertices[indices[firstIndex + 2]];

	d0[3] = b.texcoord[0] - a.texcoord[0];
	d0[4] = b.texcoord[1] - a.texcoord[1];

	d1[3] = c.texcoord[0] - a.texcoord[0];
	d1[4] = c.texcoord[1] - a.texcoord[1];

	float area = d0[3] * d1[4] - d0[4] * d1[3];

	if (area >= 0)
	{
		return false;
	}

	return true;
}

void Surface::duplicateMirroredVertexes()
{
	TangentVertex* tverts = reinterpret_cast<TangentVertex*>(alloca(vertices.size() * sizeof(TangentVertex)));
	memset(tverts, 0, vertices.size() * sizeof(TangentVertex));

	// determine texture polarity of each surface

	// mark each vert with the polarities it uses
	for (std::size_t i = 0; i < indices.size(); i += 3)
	{
		int	polarity = getFaceNegativePolarity(i) ? 1 : 0;

		for (std::size_t j = 0; j < 3 ; ++j)
		{
			tverts[indices[i+j]].polarityUsed[polarity] = true;
		}
	}

	// now create new verts as needed
	std::size_t totalVerts = vertices.size();

	for (std::size_t i = 0; i < vertices.size(); ++i)
	{
		TangentVertex& vert = tverts[i];

		if (vert.polarityUsed[0] && vert.polarityUsed[1])
		{
			vert.negativeRemap = static_cast<int>(totalVerts);
			totalVerts++;
		}
	}

	mirroredVerts.resize(totalVerts - vertices.size());

	// now create the new list
	if (totalVerts == vertices.size())
	{
		mirroredVerts.clear();
		return;
	}

	std::size_t numVertsOld = vertices.size();
	vertices.resize(totalVerts);

	// create the duplicates
	std::size_t numMirror = 0;

	for (std::size_t i = 0 ; i < numVertsOld; ++i)
	{
		int j = tverts[i].negativeRemap;

		if (j)
		{
			vertices[j] = vertices[i];
			mirroredVerts[numMirror] = static_cast<int>(i);
			numMirror++;
		}
	}

	// change the indexes
	for (std::size_t i = 0; i < indices.size(); ++i)
	{
		if (tverts[indices[i]].negativeRemap && getFaceNegativePolarity(3*(i/3)))
		{
			indices[i] = tverts[indices[i]].negativeRemap;
		}
	}
}

void Surface::createDupVerts()
{
	int* remap = (int*)alloca(vertices.size()*sizeof(int));

	// initialize vertex remap in case there are unused verts
	for (std::size_t i = 0; i < vertices.size(); ++i)
	{
		remap[i] = static_cast<int>(i);
	}

	// set the remap based on how the silhouette indexes are remapped
	for (std::size_t i = 0; i < indices.size(); ++i)
	{
		remap[indices[i]] = silIndexes[i];
	}

	// create duplicate vertex index based on the vertex remap
	dupVerts.resize(vertices.size() * 2);

	std::size_t numDupVerts = 0;

	for (std::size_t i = 0; i < vertices.size(); ++i)
	{
		if (remap[i] != i)
		{
			dupVerts[numDupVerts * 2 + 0] = static_cast<int>(i);
			dupVerts[numDupVerts * 2 + 1] = remap[i];
			numDupVerts++;
		}
	}

	dupVerts.resize(numDupVerts * 2);
}

void Surface::cleanupTriangles(bool createNormals, bool identifySilEdgesFlag, bool useUnsmoothedTangents)
{
	if (!rangeCheckIndexes()) return;

	createSilIndexes();

//	R_RemoveDuplicatedTriangles( tri );	// this may remove valid overlapped transparent triangles

	removeDegenerateTriangles();

	testDegenerateTextureSpace();

//	R_RemoveUnusedVerts( tri );

	if (identifySilEdgesFlag)
	{
		identifySilEdges(true);	// assume it is non-deformable, and omit coplanar edges
	}

	// bust vertexes that share a mirrored edge into separate vertexes
	duplicateMirroredVertexes();

	// optimize the index order (not working?)
//	R_OrderIndexes( tri->numIndexes, tri->indexes );

	createDupVerts();

	calcBounds();
	
	/*if ( useUnsmoothedTangents ) {
		R_BuildDominantTris( tri );
		R_DeriveUnsmoothedTangents( tri );
	} else if ( !createNormals ) {
		R_DeriveFacePlanes( tri );
		R_DeriveTangentsWithoutNormals( tri );
	} else {
		R_DeriveTangents( tri );
	}
	*/
}

} // namespace
