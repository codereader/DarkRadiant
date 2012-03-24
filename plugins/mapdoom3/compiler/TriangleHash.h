#pragma once

#include "math/Vector3.h"
#include "math/AABB.h"
#include <boost/shared_ptr.hpp>
#include "ProcFile.h"
#include <list>

namespace map
{

#define	HASH_BINS		16
#define	SNAP_FRACTIONS	32

#define	VERTEX_EPSILON	( 1.0 / SNAP_FRACTIONS )
#define	COLINEAR_EPSILON	( 1.8 * VERTEX_EPSILON )

struct HashVert
{
	struct HashVert* next;
	Vector3	v;
	int		iv[3];
};

class TriangleHash
{
public:
	AABB		_hashBounds;
	Vector3		_hashScale;
	HashVert*	_hashVerts[HASH_BINS][HASH_BINS][HASH_BINS];
	std::size_t	_numHashVerts;
	std::size_t _numTotalVerts;
	int			_hashIntMins[3];
	int			_hashIntScale[3];

public:
	TriangleHash() :
		_numHashVerts(0),
		_numTotalVerts(0)
	{
		// clear the hash tables
		memset(_hashVerts, 0, sizeof(_hashVerts));
	}

	void calculateBounds(const ProcArea::OptimizeGroups& groups)
	{
		//_hashBounds = AABB();

		for (ProcArea::OptimizeGroups::const_iterator group = groups.begin();
			 group != groups.end(); ++group)
		{
			for (ProcTris::const_iterator a = group->triList.begin(); a != group->triList.end(); ++a)
			{
				_hashBounds.includePoint(a->v[0].vertex);
				_hashBounds.includePoint(a->v[1].vertex);
				_hashBounds.includePoint(a->v[2].vertex);
			}
		}
	}

	void spreadHashBounds()
	{
		Vector3 min = _hashBounds.origin - _hashBounds.extents;
		Vector3 max = _hashBounds.origin + _hashBounds.extents;

		// spread the bounds so it will never have a zero size
		for (std::size_t i = 0; i < 3; ++i)
		{
			min[i] = floor(min[i] - 1);
			max[i] = ceil(max[i] + 1);
		}

		_hashBounds = AABB::createFromMinMax(min, max);

		for (std::size_t i = 0; i < 3; ++i)
		{
			_hashIntMins[i] = static_cast<int>(min[i] * SNAP_FRACTIONS);

			_hashScale[i] = _hashBounds.extents[i] / HASH_BINS;
			_hashIntScale[i] = static_cast<int>(_hashScale[i] * SNAP_FRACTIONS);

			if (_hashIntScale[i] < 1) 
			{
				_hashIntScale[i] = 1;
			}
		}
	}

	void hashTriangles(ProcArea::OptimizeGroups& groups)
	{
		// add all the points to the hash buckets
		for (ProcArea::OptimizeGroups::iterator group = groups.begin();
			 group != groups.end(); ++group)
		{
			// don't create tjunctions against discrete surfaces (blood decals, etc)
			if (group->material && group->material->isDiscrete())
			{
				continue;
			}

			for (ProcTris::iterator a = group->triList.begin(); a != group->triList.end(); ++a)
			{
				for (std::size_t vert = 0; vert < 3; ++vert)
				{
					a->hashVert[vert] = getHashVert(a->v[vert].vertex);
				}
			}
		}
	}

	// Also modifies the original vert to the snapped value
	HashVert* getHashVert(Vector3& vertex)
	{
		int		iv[3];
		int		block[3];
		std::size_t i;

		_numTotalVerts++;

		// snap the vert to integral values
		for (i = 0 ; i < 3 ; i++ )
		{
			iv[i] = static_cast<int>(floor( (vertex[i] + 0.5/SNAP_FRACTIONS ) * SNAP_FRACTIONS ));
			block[i] = ( iv[i] - _hashIntMins[i] ) / _hashIntScale[i];

			if (block[i] < 0)
			{
				block[i] = 0;
			}
			else if (block[i] >= HASH_BINS)
			{
				block[i] = HASH_BINS - 1;
			}
		}

		// see if a vertex near enough already exists
		// this could still fail to find a near neighbor right at the hash block boundary
		for (HashVert* hv = _hashVerts[block[0]][block[1]][block[2]]; hv; hv = hv->next)
		{
			std::size_t i = 0;

			for (i = 0; i < 3; ++i)
			{
				int	d = hv->iv[i] - iv[i];

				if (d < -1 || d > 1)
				{
					break;
				}
			}

			if ( i == 3 )
			{
				vertex = hv->v;
				return hv;
			}
		}

		// create a new one 
		HashVert* hv = new HashVert;

		hv->next = _hashVerts[block[0]][block[1]][block[2]];
		_hashVerts[block[0]][block[1]][block[2]] = hv;

		hv->iv[0] = iv[0];
		hv->iv[1] = iv[1];
		hv->iv[2] = iv[2];

		hv->v[0] = static_cast<float>(iv[0] / SNAP_FRACTIONS);
		hv->v[1] = static_cast<float>(iv[1] / SNAP_FRACTIONS);
		hv->v[2] = static_cast<float>(iv[2] / SNAP_FRACTIONS);

		vertex = hv->v;

		_numHashVerts++;

		return hv;
	}

	// Adds two new ProcTris to the front of the fixed list if the hashVert is on an edge of 
	// the given mapTri (returns true), otherwise does nothing (and returns false).
	bool fixTriangleAgainstHashVert(const ProcTri& a, HashVert* hv, std::list<ProcTri>& fixed)
	{
		const Vector3& v = hv->v;

		// if the triangle already has this hashVert as a vert,
		// it can't be split by it
		if (a.hashVert[0] == hv || a.hashVert[1] == hv || a.hashVert[2] == hv )
		{
			return false;
		}

		// we probably should find the edge that the vertex is closest to.
		// it is possible to be < 1 unit away from multiple
		// edges, but we only want to split by one of them
		for (std::size_t i = 0; i < 3; ++i)
		{
			const ArbitraryMeshVertex& v1 = a.v[i];
			const ArbitraryMeshVertex& v2 = a.v[(i+1) % 3];
			const ArbitraryMeshVertex& v3 = a.v[(i+2) % 3];

			Vector3 dir = v2.vertex - v1.vertex;

			float len = dir.normalise();

			// if it is close to one of the edge vertexes, skip it
			Vector3 temp = v - v1.vertex;
			
			float d = temp.dot(dir);

			if (d <= 0 || d >= len)
			{
				continue;
			}

			// make sure it is on the line
			temp = v1.vertex + dir * d;
			temp -= v;
			float off = temp.getLength();

			if (off <= -COLINEAR_EPSILON || off >= COLINEAR_EPSILON) 
			{
				continue;
			}

			// take the x/y/z from the splitter,
			// but interpolate everything else from the original tri
			float frac = d / len;

			TexCoord2f texcoord(
				v1.texcoord[0] + frac * (v2.texcoord[0] - v1.texcoord[0]),
				v1.texcoord[1] + frac * (v2.texcoord[1] - v1.texcoord[1])
			);

			Vector3 normal(
				v1.normal[0] + frac * (v2.normal[0] - v1.normal[0]),
				v1.normal[1] + frac * (v2.normal[1] - v1.normal[1]),
				v1.normal[2] + frac * (v2.normal[2] - v1.normal[2])
			);

			normal.normalise();

			ArbitraryMeshVertex split(v, normal, texcoord);

			// split the tri
			ProcTri new1(a);
			new1.v[(i + 1) % 3] = split;
			new1.hashVert[(i + 1) % 3] = hv;

			ProcTri new2(a);
			new2.v[i] = split;
			new2.hashVert[i] = hv;

			Plane3 plane1(new1.hashVert[0]->v, new1.hashVert[1]->v, new1.hashVert[2]->v);
			Plane3 plane2(new2.hashVert[0]->v, new2.hashVert[1]->v, new2.hashVert[2]->v);

			d = plane1.normal().dot(plane2.normal());

			// if the two split triangle's normals don't face the same way,
			// it should not be split
			if (d <= 0) 
			{
				continue;
			}

			fixed.push_front(new1);
			fixed.push_front(new2);

			return true;
		}


		return false;
	}

	// Potentially splits a triangle into a list of triangles based on tjunctions
	void fixTriangleAgainstHash(const ProcTri& tri, ProcTris& newList)
	{
		// if this triangle is degenerate after point snapping,
		// do nothing (this shouldn't happen, because they should
		// be removed as they are hashed)
		if (tri.hashVert[0] == tri.hashVert[1] || 
			tri.hashVert[0] == tri.hashVert[2] || 
			tri.hashVert[1] == tri.hashVert[2])
		{
			return;
		}

		int blocks[2][3];
		getHashBlocksForTri(tri, blocks);

		std::list<ProcTri> fixed(1, tri);

		for (std::size_t i = blocks[0][0]; i <= blocks[1][0]; ++i)
		{
			for (std::size_t j = blocks[0][1]; j <= blocks[1][1]; ++j)
			{
				for (std::size_t k = blocks[0][2]; k <= blocks[1][2]; ++k)
				{
					for (HashVert* hv = _hashVerts[i][j][k]; hv; hv = hv->next)
					{
						// fix all triangles in the list against this point
						std::list<ProcTri>::iterator test = fixed.begin();

						while (test != fixed.end())
						{
							if (fixTriangleAgainstHashVert(*test, hv, fixed))
							{
								// cut into two triangles, they were added to the front of the fixed list already
								// remove the old triangle, and increase the iterator to ensure it stays valid
								fixed.erase(test++);
							}
							else 
							{
								++test; // leave the triangle where it is
							}
						}
					}
				}
			}
		}

		if (!fixed.empty())
		{
			newList.insert(newList.end(), fixed.begin(), fixed.end());
		}
	}

	// Returns an inclusive bounding box of hash bins that should hold the triangle
	void getHashBlocksForTri(const ProcTri& tri, int blocks[2][3])
	{
		AABB bounds;

		bounds.includePoint(tri.v[0].vertex);
		bounds.includePoint(tri.v[1].vertex);
		bounds.includePoint(tri.v[2].vertex);

		Vector3 min = bounds.origin - bounds.extents;
		Vector3 max = bounds.origin + bounds.extents;

		Vector3 hashBoundsMin = _hashBounds.origin - _hashBounds.extents;
		Vector3 hashBoundsMax = _hashBounds.origin + _hashBounds.extents;

		// add a 1.0 slop margin on each side
		for (std::size_t i = 0; i < 3; ++i) 
		{
			blocks[0][i] = static_cast<int>((min[i] - 1.0 - hashBoundsMin[i]) / _hashScale[i]);

			if (blocks[0][i] < 0)
			{
				blocks[0][i] = 0;
			} 
			else if (blocks[0][i] >= HASH_BINS)
			{
				blocks[0][i] = HASH_BINS - 1;
			}

			blocks[1][i] = static_cast<int>((max[i] + 1.0 - hashBoundsMin[i]) / _hashScale[i]);

			if (blocks[1][i] < 0)
			{
				blocks[1][i] = 0;
			} 
			else if (blocks[1][i] >= HASH_BINS)
			{
				blocks[1][i] = HASH_BINS - 1;
			}
		}
	}
};
typedef boost::shared_ptr<TriangleHash> TriangleHashPtr;

} // namespace
