#ifndef MD5MODEL_H_
#define MD5MODEL_H_

#include "Cullable.h"
#include "Bounded.h"
#include "math/aabb.h"
#include <vector>
#include "generic/callbackfwd.h"

#include "MD5Surface.h"

namespace md5 {

// generic model node
class MD5Model :
	public Cullable,
	public Bounded
{
	typedef std::vector<MD5SurfacePtr> SurfaceList;
	SurfaceList _surfaces;

	AABB _aabb_local;
public:
	Callback _lightsChanged;

	typedef SurfaceList::const_iterator const_iterator;

	// Public iterator-related methods
	const_iterator begin() const;
	const_iterator end() const;
	std::size_t size() const;

	// Creates a new MD5Surface, adds it to the local list and returns the reference  
	md5::MD5Surface& newSurface();

	void updateAABB();

	VolumeIntersectionValue intersectVolume(
			const VolumeTest& test, const Matrix4& localToWorld) const;

	virtual const AABB& localAABB() const;

	void testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld);
}; // class MD5Model

} // namespace md5

#endif /*MD5MODEL_H_*/
