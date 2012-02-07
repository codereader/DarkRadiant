#include "Surface.h"

#include <map>
#include "itextstream.h"
#include "render/ArbitraryMeshVertex.h"

namespace map
{

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

namespace
{
	// typedefs needed to simulate the idHashIndex class
	typedef std::multimap<int, std::size_t> IndexLookupMap;
	typedef std::pair<typename IndexLookupMap::const_iterator, 
					  typename IndexLookupMap::const_iterator> Range;
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

void Surface::cleanupTriangles(bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents)
{
	if (!rangeCheckIndexes()) return;

	createSilIndexes();

//	R_RemoveDuplicatedTriangles( tri );	// this may remove valid overlapped transparent triangles

	/*R_RemoveDegenerateTriangles( tri );

	R_TestDegenerateTextureSpace( tri );

//	R_RemoveUnusedVerts( tri );

	if ( identifySilEdges ) {
		R_IdentifySilEdges( tri, true );	// assume it is non-deformable, and omit coplanar edges
	}

	// bust vertexes that share a mirrored edge into separate vertexes
	R_DuplicateMirroredVertexes( tri );

	// optimize the index order (not working?)
//	R_OrderIndexes( tri->numIndexes, tri->indexes );

	R_CreateDupVerts( tri );

	R_BoundTriSurf( tri );

	if ( useUnsmoothedTangents ) {
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
