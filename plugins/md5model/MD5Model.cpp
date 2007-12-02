#include "MD5Model.h"

namespace md5 {

MD5Model::const_iterator MD5Model::begin() const {
	return _surfaces.begin();
}

MD5Model::const_iterator MD5Model::end() const {
	return _surfaces.end();
}

std::size_t MD5Model::size() const {
	return _surfaces.size();
}

MD5Surface& MD5Model::newSurface() {
	_surfaces.push_back(md5::MD5SurfacePtr(new md5::MD5Surface));
	return *_surfaces.back();
}

void MD5Model::updateAABB() {
	_aabb_local = AABB();
	for(SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i) {
		_aabb_local.includeAABB((*i)->localAABB());
	}
}

VolumeIntersectionValue MD5Model::intersectVolume(
	const VolumeTest& test, const Matrix4& localToWorld) const
{
	return test.TestAABB(_aabb_local, localToWorld);
}

const AABB& MD5Model::localAABB() const {
	return _aabb_local;
}

void MD5Model::testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld) {
	for (SurfaceList::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i) {
		if ((*i)->intersectVolume(test.getVolume(), localToWorld) != c_volumeOutside) {
			(*i)->testSelect(selector, test, localToWorld);
		}
	}
}

} // namespace md5
