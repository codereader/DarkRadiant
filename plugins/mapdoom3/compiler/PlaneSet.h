#pragma once

#include <map>
#include "math/Plane3.h"

namespace map
{

// A planeset adds all incoming Plane3 objects into a vector, using the 
// plane's dist() value to look up the corresponding index in the vector.
class PlaneSet
{
private:
	// A mapping of (plane hash) => (vector index)
	typedef std::multimap<int, std::size_t> IndexLookupMap;
	typedef std::pair<typename IndexLookupMap::const_iterator, 
					  typename IndexLookupMap::const_iterator> Range;

	IndexLookupMap _hashToIndex;

	typedef std::vector<Plane3> PlaneList;
	PlaneList _list;

public:
	enum PlaneType
	{
		// plane types
		PLANETYPE_X					= 0,
		PLANETYPE_Y					= 1,
		PLANETYPE_Z					= 2,
		PLANETYPE_NEGX				= 3,
		PLANETYPE_NEGY				= 4,
		PLANETYPE_NEGZ				= 5,
		PLANETYPE_TRUEAXIAL			= 6,	// all types < 6 are true axial planes
		PLANETYPE_ZEROX				= 6,
		PLANETYPE_ZEROY				= 7,
		PLANETYPE_ZEROZ				= 8,
		PLANETYPE_NONAXIAL			= 9,
	};

	const Plane3& getPlane(std::size_t planeNum) const
	{
		return _list[planeNum];
	}

	std::size_t size() const
	{
		return _list.size();
	}

	// Returns the index of the plane3, which can be the index of an existing plane
	// if its normal and distance are equal respecting the given epsilon
	std::size_t findOrInsertPlane(const Plane3& plane, double epsNormal, double epsDist)
	{
		assert(epsDist <= 0.125f);

		// Use the plane's distance/8 as hash key
		int hashKey = static_cast<int>(fabs(plane.dist())*0.125);

		// The hash is not exact, so use up to three hashes to find an equivalent plane
		for (int border = -1; border <= 1; border++ )
		{
			Range range = _hashToIndex.equal_range(hashKey + border);

			for (IndexLookupMap::const_iterator i = range.first; i != range.second; ++i)
			{
				const Plane3& candidate = _list[i->second];

				if (float_equal_epsilon(candidate.dist(), plane.dist(), epsDist) &&
					candidate.normal().isEqual(plane.normal(), epsNormal))
				{
					return i->second;
				}
			}
		}

		// Plane not yet existing => classify it
		PlaneType type = getPlaneType(plane);

		if (type >= PLANETYPE_NEGX && type < PLANETYPE_TRUEAXIAL)
		{
			// Insert flipped plane first
			_list.push_back(-plane);
			_hashToIndex.insert(IndexLookupMap::value_type(hashKey, _list.size() - 1));

			_list.push_back(plane);

			std::size_t index = _list.size() - 1; // will be returned
			_hashToIndex.insert(IndexLookupMap::value_type(hashKey, index));
			
			return index;
		}
		else
		{
			_list.push_back(plane);

			std::size_t index = _list.size() - 1; // will be returned
			_hashToIndex.insert(IndexLookupMap::value_type(hashKey, index));

			_list.push_back(-plane);
			_hashToIndex.insert(IndexLookupMap::value_type(hashKey, _list.size() - 1));

			return index;
		}
	}

	// Classifies the given plane
	static PlaneType getPlaneType(const Plane3& plane)
	{
		const Vector3& normal = plane.normal();

		if ( normal[0] == 0.0f ) {
			if ( normal[1] == 0.0f ) {
				return normal[2] > 0.0f ? PLANETYPE_Z : PLANETYPE_NEGZ;
			}
			else if ( normal[2] == 0.0f ) {
				return normal[1] > 0.0f ? PLANETYPE_Y : PLANETYPE_NEGY;
			}
			else {
				return PLANETYPE_ZEROX;
			}
		}
		else if ( normal[1] == 0.0f ) {
			if ( normal[2] == 0.0f ) {
				return normal[0] > 0.0f ? PLANETYPE_X : PLANETYPE_NEGX;
			}
			else {
				return PLANETYPE_ZEROY;
			}
		}
		else if ( normal[2] == 0.0f ) {
			return PLANETYPE_ZEROZ;
		}
		else {
			return PLANETYPE_NONAXIAL;
		}
	}
};

} // namespace
